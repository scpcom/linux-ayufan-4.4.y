#if defined(CONFIG_SYNO_LSP_ARMADA_16_12)
/*
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
 * Driver for Marvell Xenon SDHC as a platform device
#else  // CONFIG_SYNO_LSP_ARMADA_17_02_02
 * Driver for Marvell SOCP Xenon SDHC as a platform device
#endif // CONFIG_SYNO_LSP_ARMADA_17_02_02
 *
 * Copyright (C) 2016 Marvell, All Rights Reserved.
 *
 * Author:	Hu Ziji <huziji@marvell.com>
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
 * Date:	2016-8-24
#else  // CONFIG_SYNO_LSP_ARMADA_17_02_02
 * Date:	2016-7-30
#endif // CONFIG_SYNO_LSP_ARMADA_17_02_02
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * Inspired by Jisheng Zhang <jszhang@marvell.com>
 * Special thanks to Video BG4 project team.
 */

#include <linux/delay.h>
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
#include <linux/module.h>
#include <linux/of.h>

#include "sdhci-pltfm.h"
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
#include "sdhci.h"
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
#include "sdhci-xenon.h"

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
static int enable_xenon_internal_clk(struct sdhci_host *host)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
/*
 * Xenon Specific Initialization Operations
 */
static inline void xenon_set_tuning_count(struct sdhci_host *host,
				unsigned int count)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	u32 reg;
	u8 timeout;

	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_INT_EN;
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);
	/* Wait max 20 ms */
	timeout = 20;
	while (!((reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL))
			& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			pr_err("%s: Internal clock never stabilised.\n",
			       mmc_hostname(host->mmc));
			return -ETIMEDOUT;
		}
		timeout--;
		mdelay(1);
	}
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	/* A valid count value */
	host->tuning_count = 1 << (count - 1);
}
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	return 0;
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
/*
 * Current driver can only support Tuning Mode 1.
 * Tuning timer is only setup only tuning_mode == Tuning Mode 1.
 * Thus host->tuning_mode has to be forced as Tuning Mode 1.
 */
static inline void xenon_set_tuning_mode(struct sdhci_host *host)
{
	host->tuning_mode = SDHCI_TUNING_MODE_1;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

/* Set SDCLK-off-while-idle */
static void xenon_set_sdclk_off_idle(struct sdhci_host *host,
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
				     unsigned char sdhc_id, bool enable)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
			unsigned char slot_idx, bool enable)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
	u32 reg;
	u32 mask;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	reg = sdhci_readl(host, XENON_SYS_OP_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = sdhci_readl(host, SDHCI_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	/* Get the bit shift basing on the SDHC index */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	mask = (0x1 << (XENON_SDCLK_IDLEOFF_ENABLE_SHIFT + sdhc_id));
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	mask = (0x1 << (SDHCI_SDCLK_IDLEOFF_ENABLE_SHIFT + sdhc_id));
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	/* Get the bit shift basing on the slot index */
	mask = (0x1 << (SDCLK_IDLEOFF_ENABLE_SHIFT + slot_idx));
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (enable)
		reg |= mask;
	else
		reg &= ~mask;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	sdhci_writel(host, reg, XENON_SYS_OP_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	sdhci_writel(host, reg, SDHCI_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

/* Enable/Disable the Auto Clock Gating function */
static void xenon_set_acg(struct sdhci_host *host, bool enable)
{
	u32 reg;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	reg = sdhci_readl(host, XENON_SYS_OP_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = sdhci_readl(host, SDHCI_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (enable)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		reg &= ~XENON_AUTO_CLKGATE_DISABLE_MASK;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		reg &= ~SDHCI_AUTO_CLKGATE_DISABLE_MASK;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		reg &= ~AUTO_CLKGATE_DISABLE_MASK;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	else
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		reg |= XENON_AUTO_CLKGATE_DISABLE_MASK;
	sdhci_writel(host, reg, XENON_SYS_OP_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		reg |= SDHCI_AUTO_CLKGATE_DISABLE_MASK;
	sdhci_writel(host, reg, SDHCI_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		reg |= AUTO_CLKGATE_DISABLE_MASK;
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
/* Enable this SDHC */
static void xenon_enable_sdhc(struct sdhci_host *host,
			      unsigned char sdhc_id)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
/* Enable this slot */
static void xenon_enable_slot(struct sdhci_host *host,
			unsigned char slot_idx)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
	u32 reg;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	reg = sdhci_readl(host, XENON_SYS_OP_CTRL);
	reg |= (BIT(sdhc_id) << XENON_SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, XENON_SYS_OP_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = sdhci_readl(host, SDHCI_SYS_OP_CTRL);
	reg |= (BIT(sdhc_id) << SDHCI_SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHCI_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	reg |= ((0x1 << slot_idx) << SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	/*
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	 * Manually set the flag which all the card types require,
#else  // CONFIG_SYNO_LSP_ARMADA_17_02_02
	 * Manually set the flag which all the slots require,
#endif // CONFIG_SYNO_LSP_ARMADA_17_02_02
	 * including SD, eMMC, SDIO
	 */
	host->mmc->caps |= MMC_CAP_WAIT_WHILE_BUSY;
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
/* Disable this SDHC */
static void xenon_disable_sdhc(struct sdhci_host *host,
			       unsigned char sdhc_id)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
/* Disable this slot */
static void xenon_disable_slot(struct sdhci_host *host,
			unsigned char slot_idx)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
	u32 reg;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	reg = sdhci_readl(host, XENON_SYS_OP_CTRL);
	reg &= ~(BIT(sdhc_id) << XENON_SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, XENON_SYS_OP_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = sdhci_readl(host, SDHCI_SYS_OP_CTRL);
	reg &= ~(BIT(sdhc_id) << SDHCI_SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHCI_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	reg &= ~((0x1 << slot_idx) << SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

/* Enable Parallel Transfer Mode */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
static void xenon_enable_sdhc_parallel_tran(struct sdhci_host *host,
					    unsigned char sdhc_id)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
static void xenon_enable_slot_parallel_tran(struct sdhci_host *host,
			unsigned char slot_idx)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
	u32 reg;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	reg = sdhci_readl(host, XENON_SYS_EXT_OP_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = sdhci_readl(host, SDHCI_SYS_EXT_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg |= BIT(sdhc_id);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	sdhci_writel(host, reg, XENON_SYS_EXT_OP_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	sdhci_writel(host, reg, SDHCI_SYS_EXT_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	reg = sdhci_readl(host, SDHC_SYS_EXT_OP_CTRL);
	reg |= (0x1 << slot_idx);
	sdhci_writel(host, reg, SDHC_SYS_EXT_OP_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
/* Mask command conflict error */
static void xenon_mask_cmd_conflict_err(struct sdhci_host *host)
{
	u32  reg;

	reg = sdhci_readl(host, XENON_SYS_EXT_OP_CTRL);
	reg |= XENON_MASK_CMD_CONFLICT_ERR;
	sdhci_writel(host, reg, XENON_SYS_EXT_OP_CTRL);
}

static void xenon_sdhc_retune_setup(struct sdhci_host *host)
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static void xenon_sdhc_tuning_setup(struct sdhci_host *host)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
static void xenon_slot_tuning_setup(struct sdhci_host *host)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	u32 reg;

	/* Disable the Re-Tuning Request functionality */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	reg = sdhci_readl(host, XENON_SLOT_RETUNING_REQ_CTRL);
	reg &= ~XENON_RETUNING_COMPATIBLE;
	sdhci_writel(host, reg, XENON_SLOT_RETUNING_REQ_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = sdhci_readl(host, SDHCI_SLOT_RETUNING_REQ_CTRL);
	reg &= ~SDHCI_RETUNING_COMPATIBLE;
	sdhci_writel(host, reg, SDHCI_SLOT_RETUNING_REQ_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	reg = sdhci_readl(host, SDHC_SLOT_RETUNING_REQ_CTRL);
	reg &= ~RETUNING_COMPATIBLE;
	sdhci_writel(host, reg, SDHC_SLOT_RETUNING_REQ_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	/* Disable the Re-tuning Interrupt */
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	/* Disable the Re-tuning Event Signal Enable */
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);
	reg &= ~SDHCI_INT_RETUNE;
	sdhci_writel(host, reg, SDHCI_SIGNAL_ENABLE);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	reg = sdhci_readl(host, SDHCI_INT_ENABLE);
	reg &= ~SDHCI_INT_RETUNE;
	sdhci_writel(host, reg, SDHCI_INT_ENABLE);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	/* Force to use Tuning Mode 1 */
	host->tuning_mode = SDHCI_TUNING_MODE_1;
	/* Set re-tuning period */
	host->tuning_count = 1 << (priv->tuning_count - 1);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	/* Disable Auto-retuning */
	reg = sdhci_readl(host, SDHC_SLOT_AUTO_RETUNING_CTRL);
	reg &= ~ENABLE_AUTO_RETUNING;
	sdhci_writel(host, reg, SDHC_SLOT_AUTO_RETUNING_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

/*
 * Operations inside struct sdhci_ops
 */
/* Recover the Register Setting cleared during SOFTWARE_RESET_ALL */
static void sdhci_xenon_reset_exit(struct sdhci_host *host,
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
				   unsigned char sdhc_id, u8 mask)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
					unsigned char slot_idx, u8 mask)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
	/* Only SOFTWARE RESET ALL will clear the register setting */
	if (!(mask & SDHCI_RESET_ALL))
		return;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	/* Disable tuning request and auto-retuning again */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	xenon_sdhc_retune_setup(host);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	xenon_sdhc_tuning_setup(host);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	/* Disable tuning request and auto-retuing again */
	xenon_slot_tuning_setup(host);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	xenon_set_acg(host, true);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	xenon_set_sdclk_off_idle(host, sdhc_id, false);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	xenon_set_sdclk_off_idle(host, slot_idx, false);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)

	xenon_mask_cmd_conflict_err(host);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

static void sdhci_xenon_reset(struct sdhci_host *host, u8 mask)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

	sdhci_reset(host, mask);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	sdhci_xenon_reset_exit(host, priv->sdhc_id, mask);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	sdhci_xenon_reset_exit(host, priv->slot_idx, mask);
}

static void xenon_platform_init(struct sdhci_host *host)
{
	xenon_set_acg(host, false);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

/*
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
 * Xenon defines different values for HS200 and HS400
#else  // CONFIG_SYNO_LSP_ARMADA_17_02_02
 * Xenon defines different values for HS200 and SDR104
#endif // CONFIG_SYNO_LSP_ARMADA_17_02_02
 * in Host_Control_2
 */
static void xenon_set_uhs_signaling(struct sdhci_host *host,
				unsigned int timing)
{
	u16 ctrl_2;

	ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	/* Select Bus Speed Mode for host */
	ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;
	if (timing == MMC_TIMING_MMC_HS200)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		ctrl_2 |= XENON_CTRL_HS200;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		ctrl_2 |= SDHCI_XENON_CTRL_HS200;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		ctrl_2 |= XENON_SDHCI_CTRL_HS200;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	else if (timing == MMC_TIMING_UHS_SDR104)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
	else if (timing == MMC_TIMING_UHS_SDR12)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
	else if (timing == MMC_TIMING_UHS_SDR25)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
	else if (timing == MMC_TIMING_UHS_SDR50)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
	else if ((timing == MMC_TIMING_UHS_DDR50) ||
		 (timing == MMC_TIMING_MMC_DDR52))
		ctrl_2 |= SDHCI_CTRL_UHS_DDR50;
	else if (timing == MMC_TIMING_MMC_HS400)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		ctrl_2 |= XENON_CTRL_HS400;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		ctrl_2 |= SDHCI_XENON_CTRL_HS400;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		ctrl_2 |= XENON_SDHCI_CTRL_HS400;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);
}

static const struct sdhci_ops sdhci_xenon_ops = {
	.set_clock		= sdhci_set_clock,
	.set_bus_width		= sdhci_set_bus_width,
	.reset			= sdhci_xenon_reset,
	.set_uhs_signaling	= xenon_set_uhs_signaling,
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	.platform_init		= xenon_platform_init,
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	.get_max_clock		= sdhci_pltfm_clk_get_max_clock,
};

static const struct sdhci_pltfm_data sdhci_xenon_pdata = {
	.ops = &sdhci_xenon_ops,
	.quirks = SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC |
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
// do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
			SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12 |
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
			SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER |
			SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN,
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
// do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	/*
	 * Add SOC specific quirks in the above .quirks, .quirks2
	 * fields.
	 */
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
};

/*
 * Xenon Specific Operations in mmc_host_ops
 */
static void xenon_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	unsigned long flags;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	u32 reg;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	/*
	 * HS400/HS200/eMMC HS doesn't have Preset Value register.
	 * However, sdhci_set_ios will read HS400/HS200 Preset register.
	 * Disable Preset Value register for HS400/HS200.
	 * eMMC HS with preset_enabled set will trigger a bug in
	 * get_preset_value().
	 */
	spin_lock_irqsave(&host->lock, flags);
	if ((ios->timing == MMC_TIMING_MMC_HS400) ||
		(ios->timing == MMC_TIMING_MMC_HS200) ||
		(ios->timing == MMC_TIMING_MMC_HS)) {
		host->preset_enabled = false;
		host->quirks2 |= SDHCI_QUIRK2_PRESET_VALUE_BROKEN;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		host->flags &= ~SDHCI_PV_ENABLED;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

		reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		reg &= ~SDHCI_CTRL_PRESET_VAL_ENABLE;
		sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);
	} else {
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	} else
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		host->quirks2 &= ~SDHCI_QUIRK2_PRESET_VALUE_BROKEN;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	}
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	spin_unlock_irqrestore(&host->lock, flags);

	sdhci_set_ios(mmc, ios);
	xenon_phy_adj(host, ios);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	if (host->clock > XENON_DEFAULT_SDCLK_FREQ) {
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (host->clock > SDHCI_DEFAULT_SDCLK_FREQ) {
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		spin_lock_irqsave(&host->lock, flags);
		xenon_set_sdclk_off_idle(host, priv->sdhc_id, true);
		spin_unlock_irqrestore(&host->lock, flags);
	}
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (host->clock > DEFAULT_SDCLK_FREQ)
		xenon_set_sdclk_off_idle(host, priv->slot_idx, true);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
static int xenon_emmc_signal_voltage_switch(struct mmc_host *mmc,
					    struct mmc_ios *ios)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
static int __emmc_signal_voltage_switch(struct mmc_host *mmc,
				const unsigned char signal_voltage)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	unsigned char voltage = ios->signal_voltage;
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	u32 ctrl;
	unsigned char voltage_code;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	struct sdhci_host *host = mmc_priv(mmc);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	unsigned char voltage_code;
	u32 ctrl;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	if ((voltage == MMC_SIGNAL_VOLTAGE_330) ||
	    (voltage == MMC_SIGNAL_VOLTAGE_180)) {
		if (voltage == MMC_SIGNAL_VOLTAGE_330)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
			voltage_code = XENON_EMMC_VCCQ_3_3V;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
			voltage_code = SDHCI_EMMC_VCCQ_3_3V;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		else if (voltage == MMC_SIGNAL_VOLTAGE_180)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
			voltage_code = XENON_EMMC_VCCQ_1_8V;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
			voltage_code = SDHCI_EMMC_VCCQ_1_8V;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (signal_voltage == MMC_SIGNAL_VOLTAGE_330)
		voltage_code = eMMC_VCCQ_3_3V;
	else if (signal_voltage == MMC_SIGNAL_VOLTAGE_180)
		voltage_code = eMMC_VCCQ_1_8V;
	else
		return -EINVAL;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	/*
	 * This host is for eMMC, XENON self-defined
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	 * eMMC control register should be accessed
#else  // CONFIG_SYNO_LSP_ARMADA_17_02_02
	 * eMMC slot control register should be accessed
#endif // CONFIG_SYNO_LSP_ARMADA_17_02_02
	 * instead of Host Control 2
	 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		ctrl = sdhci_readl(host, XENON_SLOT_EMMC_CTRL);
		ctrl &= ~XENON_EMMC_VCCQ_MASK;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		ctrl = sdhci_readl(host, SDHCI_SLOT_EMMC_CTRL);
		ctrl &= ~SDHCI_EMMC_VCCQ_MASK;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	ctrl = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	ctrl &= ~eMMC_VCCQ_MASK;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	ctrl |= voltage_code;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		sdhci_writel(host, ctrl, XENON_SLOT_EMMC_CTRL);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		sdhci_writel(host, ctrl, SDHCI_SLOT_EMMC_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	sdhci_writel(host, ctrl, SDHC_SLOT_eMMC_CTRL);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	/* There is no standard to determine this waiting period */
	usleep_range(1000, 2000);

	/* Check whether io voltage switch is done */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		ctrl = sdhci_readl(host, XENON_SLOT_EMMC_CTRL);
		ctrl &= XENON_EMMC_VCCQ_MASK;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		ctrl = sdhci_readl(host, SDHCI_SLOT_EMMC_CTRL);
		ctrl &= SDHCI_EMMC_VCCQ_MASK;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		/*
		 * This bit is set only when regulator feeds back
		 * the voltage switch results to Xenon SDHC.
		 * However, in actaul implementation, regulator might not
		 * provide this feedback.
		 * Thus we shall not rely on this bit to determine
		 * if switch failed.
		 * If the bit is not set, just throw a message.
		 * Besides, error code should not be returned.
		 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	ctrl = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	ctrl &= eMMC_VCCQ_MASK;
	/*
	 * This bit is set only when regulator feedbacks the voltage switch
	 * results to Xenon SDHC.
	 * However, in actaul implementation, regulator might not provide
	 * this feedback.
	 * Thus we shall not rely on this bit to determine if switch failed.
	 * If the bit is not set, just throw a warning.
	 * Besides, error code should neither be returned.
	 */
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (ctrl != voltage_code)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
			dev_info(mmc_dev(mmc), "fail to detect eMMC signal voltage stable\n");
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		pr_info("%s: Xenon fail to detect eMMC signal voltage stable\n",
					mmc_hostname(mmc));
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	return 0;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	}
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

static int xenon_emmc_signal_voltage_switch(struct mmc_host *mmc,
					struct mmc_ios *ios)
{
	unsigned char voltage = ios->signal_voltage;

	if ((voltage == MMC_SIGNAL_VOLTAGE_330) ||
		(voltage == MMC_SIGNAL_VOLTAGE_180))
		return __emmc_signal_voltage_switch(mmc, voltage);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	dev_err(mmc_dev(mmc), "Unsupported signal voltage: %d\n", voltage);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	pr_err("%s: Xenon Unsupported signal voltage: %d\n",
				mmc_hostname(mmc), voltage);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	return -EINVAL;
}

static int xenon_start_signal_voltage_switch(struct mmc_host *mmc,
					     struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

	/*
	 * Before SD/SDIO set signal voltage, SD bus clock should be
	 * disabled. However, sdhci_set_clock will also disable the Internal
	 * clock in mmc_set_signal_voltage().
	 * If Internal clock is disabled, the 3.3V/1.8V bit can not be updated.
	 * Thus here manually enable internal clock.
	 *
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	 * After switch completes, it is unnecessary to disable internal clock,
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	 * After switch completes, it is unnessary to disable internal clock,
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	 * since keeping internal clock active obeys SD spec.
	 */
	enable_xenon_internal_clk(host);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
// do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (priv->card_candidate) {
		if (mmc_card_mmc(priv->card_candidate)) {
			/* Set SoC PAD register for MMC PHY voltage */
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
			xenon_soc_pad_ctrl(host, ios->signal_voltage);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	if (priv->init_card_type == MMC_TYPE_MMC)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
			return xenon_emmc_signal_voltage_switch(mmc, ios);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
// do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		}
	}
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	return sdhci_start_signal_voltage_switch(mmc, ios);
}

/*
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
 * Update card type.
 * priv->init_card_type will be used in PHY timing adjustment.
#else  // CONFIG_SYNO_LSP_ARMADA_17_02_02
 * After determining the slot is used for SDIO,
 * some addtional task is required.
#endif // CONFIG_SYNO_LSP_ARMADA_17_02_02
 */
static void xenon_init_card(struct mmc_host *mmc, struct mmc_card *card)
{
	struct sdhci_host *host = mmc_priv(mmc);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	u32 reg;
	u8 slot_idx;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	/* Update card type*/
	priv->init_card_type = card->type;
}

static int xenon_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct sdhci_host *host = mmc_priv(mmc);

	if (host->timing == MMC_TIMING_UHS_DDR50)
		return 0;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	/*
	 * Currently force Xenon driver back to support mode 1 only,
	 * even though Xenon might claim to support mode 2 or mode 3.
	 * It requires more time to test mode 2/mode 3 on more platforms.
	 */
	if (host->tuning_mode != SDHCI_TUNING_MODE_1)
		xenon_sdhc_retune_setup(host);

#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	return sdhci_execute_tuning(mmc, opcode);
}
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	/* Link the card for delay adjustment */
	priv->card_candidate = card;
	/* Set Xenon tuning */
	xenon_set_tuning_mode(host);
	xenon_set_tuning_count(host, priv->tuning_count);

	slot_idx = priv->slot_idx;
	if (!mmc_card_sdio(card)) {
		/* Re-enable the Auto-CMD12 cap flag. */
		host->quirks |= SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;
		host->flags |= SDHCI_AUTO_CMD12;

		/* Clear SDIO Card Insterted indication */
		reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
		reg &= ~(1 << (slot_idx + SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHC_SYS_CFG_INFO);

		if (mmc_card_mmc(card)) {
			mmc->caps |= MMC_CAP_NONREMOVABLE | MMC_CAP_1_8V_DDR;
			/*
			 * Force to clear BUS_TEST to
			 * skip bus_test_pre and bus_test_post
			 */
			mmc->caps &= ~MMC_CAP_BUS_WIDTH_TEST;
			mmc->caps2 |= MMC_CAP2_HS400_1_8V |
				MMC_CAP2_HC_ERASE_SZ | MMC_CAP2_PACKED_CMD;
		}
		/* Xenon SD doesn't support DDR50 tuning.*/
		if (mmc_card_sd(card))
			mmc->caps2 |= MMC_CAP2_NO_DDR50_TUNING;
	} else {
		/*
		 * Delete the Auto-CMD12 cap flag.
		 * Otherwise, when sending multi-block CMD53,
		 * Driver will set Transfer Mode Register to enable Auto CMD12.
		 * However, SDIO device cannot recognize CMD12.
		 * Thus SDHC will time-out for waiting for CMD12 response.
		 */
		host->quirks &= ~SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;
		host->flags &= ~SDHCI_AUTO_CMD12;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
static void xenon_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 reg;
	u8 sdhc_id = priv->sdhc_id;

	sdhci_enable_sdio_irq(mmc, enable);

	if (enable) {
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		/*
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
		 * Set SDIO Card Inserted indication
		 * to enable detecting SDIO async irq.
#else  // CONFIG_SYNO_LSP_ARMADA_17_02_02
		 * Set SDIO Card Insterted indication
		 * to inform that the current slot is for SDIO
#endif // CONFIG_SYNO_LSP_ARMADA_17_02_02
		 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		reg = sdhci_readl(host, XENON_SYS_CFG_INFO);
		reg |= (1 << (sdhc_id + XENON_SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, XENON_SYS_CFG_INFO);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		reg = sdhci_readl(host, SDHCI_SYS_CFG_INFO);
		reg |= (1 << (sdhc_id + SDHCI_SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHCI_SYS_CFG_INFO);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	} else {
		/* Clear SDIO Card Inserted indication */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		reg = sdhci_readl(host, XENON_SYS_CFG_INFO);
		reg &= ~(1 << (sdhc_id + XENON_SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, XENON_SYS_CFG_INFO);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		reg = sdhci_readl(host, SDHCI_SYS_CFG_INFO);
		reg &= ~(1 << (sdhc_id + SDHCI_SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHCI_SYS_CFG_INFO);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
		reg |= (1 << (slot_idx + SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHC_SYS_CFG_INFO);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	}
}

static void xenon_replace_mmc_host_ops(struct sdhci_host *host)
{
	host->mmc_host_ops.set_ios = xenon_set_ios;
	host->mmc_host_ops.start_signal_voltage_switch =
			xenon_start_signal_voltage_switch;
	host->mmc_host_ops.init_card = xenon_init_card;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	host->mmc_host_ops.execute_tuning = xenon_execute_tuning;
	host->mmc_host_ops.enable_sdio_irq = xenon_enable_sdio_irq;
}

/*
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
 * Parse Xenon specific DT properties:
 * init_card_type: check whether this SDHC is for eMMC
 * sdhc-id: the index of current SDHC.
 *	    Refer to XENON_SYS_CFG_INFO register
 * tun-count: the interval between re-tuning
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
 * Parse child node in Xenon DT.
 * Search for the following item(s):
 * - eMMC card type
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static int xenon_child_node_of_parse(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct mmc_host *mmc = host->mmc;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct device_node *child;
	int nr_child;

	priv->init_card_type = SDHCI_CARD_TYPE_UNKNOWN;

	nr_child = of_get_child_count(np);
	if (!nr_child)
		return 0;

	for_each_child_of_node(np, child) {
		if (of_device_is_compatible(child, "mmc-card"))	{
			priv->init_card_type = MMC_TYPE_MMC;
			mmc->caps |= MMC_CAP_NONREMOVABLE;

			/*
			 * Force to clear BUS_TEST to
			 * skip bus_test_pre and bus_test_post
			 */
			mmc->caps &= ~MMC_CAP_BUS_WIDTH_TEST;
			mmc->caps2 |= MMC_CAP2_HC_ERASE_SZ |
				      MMC_CAP2_PACKED_CMD |
				      MMC_CAP2_NO_SD |
				      MMC_CAP2_NO_SDIO;
		}
	}

	return 0;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static int xenon_probe_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct mmc_host *mmc = host->mmc;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	int err;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	u32 sdhc_id, nr_sdhc;
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	u32 slot_idx;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	u32 tuning_count;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	/* Disable HS200 on Armada AP806 */
	if (of_device_is_compatible(np, "marvell,armada-ap806-sdhci"))
		host->quirks2 |= SDHCI_QUIRK2_BROKEN_HS200;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	/* Standard MMC property */
	err = mmc_of_parse(mmc);
	if (err)
		return err;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	priv->init_card_type = XENON_CARD_TYPE_UNKNOWN;
	/* Check if mmc-card sub-node exists */
	if (mmc_of_parse_mmc_card(mmc)) {
		priv->init_card_type = MMC_TYPE_MMC;
		/*
		 * Force to clear BUS_TEST to
		 * skip bus_test_pre and bus_test_post
		 */
		mmc->caps &= ~MMC_CAP_BUS_WIDTH_TEST;
	}
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	/* Standard SDHCI property */
	sdhci_get_of_property(pdev);

	/*
	 * Xenon Specific property:
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	 * init_card_type: check whether this SDHC is for eMMC
	 * sdhc-id: the index of current SDHC.
	 *	    Refer to SDHCI_SYS_CFG_INFO register
	 * tun-count: the interval between re-tuning
#else  // CONFIG_SYNO_LSP_ARMADA_17_02_02
	 * slotno: the index of slot. Refer to SDHC_SYS_CFG_INFO register
	 * tuning-count: the interval between re-tuning
	 * PHY type: "sdhc phy", "emmc phy 5.0" or "emmc phy 5.1"
#endif // CONFIG_SYNO_LSP_ARMADA_17_02_02
	 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	/* Parse child node, including checking emmc type */
	err = xenon_child_node_of_parse(pdev);
	if (err)
		return err;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	priv->sdhc_id = 0x0;
	if (!of_property_read_u32(np, "marvell,xenon-sdhc-id", &sdhc_id)) {
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		nr_sdhc = sdhci_readl(host, XENON_SYS_CFG_INFO);
		nr_sdhc &= XENON_NR_SUPPORTED_SLOT_MASK;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		nr_sdhc = sdhci_readl(host, SDHCI_SYS_CFG_INFO);
		nr_sdhc &= SDHCI_NR_SUPPORTED_SLOT_MASK;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		if (unlikely(sdhc_id > nr_sdhc)) {
			dev_err(mmc_dev(mmc), "SDHC Index %d exceeds Number of SDHCs %d\n",
				sdhc_id, nr_sdhc);
			return -EINVAL;
		}
	}
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	tuning_count = XENON_DEF_TUNING_COUNT;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	tuning_count = SDHCI_DEF_TUNING_COUNT;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (!of_property_read_u32(np, "marvell,xenon-tun-count",
				  &tuning_count)) {
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		if (unlikely(tuning_count >= XENON_TMR_RETUN_NO_PRESENT)) {
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		if (unlikely(tuning_count >= SDHCI_TMR_RETUN_NO_PRESENT)) {
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
			dev_err(mmc_dev(mmc), "Wrong Re-tuning Count. Set default value %d\n",
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
				XENON_DEF_TUNING_COUNT);
			tuning_count = XENON_DEF_TUNING_COUNT;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
				SDHCI_DEF_TUNING_COUNT);
			tuning_count = SDHCI_DEF_TUNING_COUNT;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		}
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (!of_property_read_u32(np, "xenon,slotno", &slot_idx))
		priv->slot_idx = slot_idx & 0xff;
	else
		priv->slot_idx = 0x0;

	if (!of_property_read_u32(np, "xenon,tuning-count", &tuning_count)) {
		if (unlikely(tuning_count >= TMR_RETUN_NO_PRESENT)) {
			pr_err("%s: Wrong Re-tuning Count. Set default value %d\n",
				mmc_hostname(mmc), DEF_TUNING_COUNT);
			tuning_count = DEF_TUNING_COUNT;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		}
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	priv->tuning_count = tuning_count;
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		priv->tuning_count = tuning_count & 0xf;
	} else
		priv->tuning_count = DEF_TUNING_COUNT;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	return xenon_phy_parse_dt(np, host);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	err = xenon_phy_parse_dt(np, priv);
	return err;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
static int xenon_sdhc_probe(struct sdhci_host *host)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
static int xenon_slot_probe(struct sdhci_host *host)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	u8 sdhc_id = priv->sdhc_id;
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	u8 slot_idx = priv->slot_idx;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	/* Enable SDHC */
	xenon_enable_sdhc(host, sdhc_id);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	/* Enable slot */
	xenon_enable_slot(host, slot_idx);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	/* Enable ACG */
	xenon_set_acg(host, true);

	/* Enable Parallel Transfer Mode */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	xenon_enable_sdhc_parallel_tran(host, sdhc_id);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	xenon_enable_slot_parallel_tran(host, slot_idx);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	xenon_mask_cmd_conflict_err(host);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	/* Set tuning functionality of this SDHC */
	xenon_sdhc_tuning_setup(host);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	priv->timing = MMC_TIMING_FAKE;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	return 0;
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
static void xenon_sdhc_remove(struct sdhci_host *host)
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
static void xenon_slot_remove(struct sdhci_host *host)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	u8 sdhc_id = priv->sdhc_id;
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	u8 slot_idx = priv->slot_idx;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	/* disable SDHC */
	xenon_disable_sdhc(host, sdhc_id);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	/* disable slot */
	xenon_disable_slot(host, slot_idx);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
}

static int sdhci_xenon_probe(struct platform_device *pdev)
{
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_host *host;
	struct sdhci_xenon_priv *priv;
	int err;

	host = sdhci_pltfm_init(pdev, &sdhci_xenon_pdata,
		sizeof(struct sdhci_xenon_priv));
	if (IS_ERR(host))
		return PTR_ERR(host);

	pltfm_host = sdhci_priv(host);
	priv = sdhci_pltfm_priv(pltfm_host);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	xenon_set_acg(host, false);

#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	/*
	 * Link Xenon specific mmc_host_ops function,
	 * to replace standard ones in sdhci_ops.
	 */
	xenon_replace_mmc_host_ops(host);

	pltfm_host->clk = devm_clk_get(&pdev->dev, "core");
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	if (IS_ERR(pltfm_host->clk)) {
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (!IS_ERR(pltfm_host->clk)) {
		err = clk_prepare_enable(pltfm_host->clk);
		if (err)
			goto free_pltfm;
	} else if (PTR_ERR(pltfm_host->clk) == -EPROBE_DEFER) {
		err = -EPROBE_DEFER;
		goto free_pltfm;
	} else {
		pr_err("%s: Failed to setup input clk.\n",
			mmc_hostname(host->mmc));
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		err = PTR_ERR(pltfm_host->clk);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
		dev_err(&pdev->dev, "Failed to setup input clk: %d\n", err);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		goto free_pltfm;
	}
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	err = clk_prepare_enable(pltfm_host->clk);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	/*
	 * Some SOCs require additional clock to
	 * manage AXI bus clock.
	 * It is optional.
	 */
	priv->axi_clk = devm_clk_get(&pdev->dev, "axi");
	if (!IS_ERR(priv->axi_clk)) {
		err = clk_prepare_enable(priv->axi_clk);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		if (err)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
		goto free_pltfm;
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
			goto err_clk;
	}
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	err = mmc_of_parse(host->mmc);
	if (err)
		goto err_clk;

	sdhci_get_of_property(pdev);

	/* Xenon specific dt parse */
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	err = xenon_probe_dt(pdev);
	if (err)
		goto err_clk;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	err = xenon_sdhc_probe(host);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	err = xenon_slot_probe(host);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (err)
		goto err_clk;

	err = sdhci_add_host(host);
	if (err)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
		goto remove_sdhc;
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		goto remove_slot;

	/* Set tuning functionality of this slot */
	xenon_slot_tuning_setup(host);

	/* Initialize SoC PAD register for MMC PHY voltage
	 * For eMMC, it is set to 1.8V
	 * For SD/SDIO, it is set to 3.3V
	 */
	xenon_soc_pad_ctrl(host, MMC_SIGNAL_VOLTAGE_330);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	return 0;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
remove_sdhc:
	xenon_sdhc_remove(host);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
remove_slot:
	xenon_slot_remove(host);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
err_clk:
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
// do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (!IS_ERR(pltfm_host->clk))
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		clk_disable_unprepare(pltfm_host->clk);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
// do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	if (!IS_ERR(priv->axi_clk))
		clk_disable_unprepare(priv->axi_clk);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
free_pltfm:
	sdhci_pltfm_free(pdev);
	return err;
}

static int sdhci_xenon_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	int dead = (readl(host->ioaddr + SDHCI_INT_STATUS) == 0xFFFFFFFF);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int dead = (readl(host->ioaddr + SDHCI_INT_STATUS) == 0xffffffff);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
	xenon_sdhc_remove(host);
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	xenon_slot_remove(host);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	sdhci_remove_host(host, 0);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	sdhci_remove_host(host, dead);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	clk_disable_unprepare(pltfm_host->clk);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	clk_disable_unprepare(priv->axi_clk);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */

	sdhci_pltfm_free(pdev);

	return 0;
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
#ifdef CONFIG_PM
static int sdhci_xenon_suspend(struct device *dev)
{
	int ret;
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);

	ret = sdhci_suspend_host(host);

	if (pltfm_host->clk)
		clk_disable_unprepare(pltfm_host->clk);

	return ret;
}

static int sdhci_xenon_resume(struct device *dev)
{
	int ret;
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);

	if (pltfm_host->clk)
		ret = clk_prepare_enable(pltfm_host->clk);

	ret = xenon_sdhc_probe(host);

	/* Initialize SoC PAD register for MMC PHY voltage
	 * For eMMC, it is set to 1.8V
	 * For SD/SDIO, it is set to 3.3V
	 */
	xenon_soc_pad_ctrl(host, MMC_SIGNAL_VOLTAGE_330);

	ret = sdhci_resume_host(host);

	return ret;
}

static const struct dev_pm_ops sdhci_xenon_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(sdhci_xenon_suspend, sdhci_xenon_resume)
};
#endif

#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static const struct of_device_id sdhci_xenon_dt_ids[] = {
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	{ .compatible = "marvell,armada-ap806-sdhci",},
	{ .compatible = "marvell,armada-cp110-sdhci",},
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	{ .compatible = "marvell,armada8k-sdhci",},
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	{ .compatible = "marvell,armada-3700-sdhci",},
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	{ .compatible = "marvell,xenon-sdhci",},
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_xenon_dt_ids);

static struct platform_driver sdhci_xenon_driver = {
	.driver	= {
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
		.name	= "xenon-sdhci",
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		.name	= "mv-xenon-sdhci",
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		.of_match_table = sdhci_xenon_dt_ids,
#if defined(CONFIG_SYNO_LSP_ARMADA_17_02_02)
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
#ifdef CONFIG_PM
		.pm = &sdhci_xenon_pmops,
#endif
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		.pm = &sdhci_pltfm_pmops,
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#else /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
		.pm = SDHCI_PLTFM_PMOPS,
#endif /* CONFIG_SYNO_LSP_ARMADA_17_02_02 */
	},
	.probe	= sdhci_xenon_probe,
	.remove	= sdhci_xenon_remove,
};

module_platform_driver(sdhci_xenon_driver);

MODULE_DESCRIPTION("SDHCI platform driver for Marvell Xenon SDHC");
MODULE_AUTHOR("Hu Ziji <huziji@marvell.com>");
MODULE_LICENSE("GPL v2");
#endif /* CONFIG_SYNO_LSP_ARMADA_16_12 */
