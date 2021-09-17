/****************************************************************************
 *	Flash Memory Controller v100 Device Driver
 *	Copyright (c) 2014 - 2015 by Hisilicon
 *	All rights reserved.
 * ***
 *	Create by hisilicon
 *
 ****************************************************************************/
#include <mach/hardware.h>

/****************************************************************************/
/* bit[7]	0: 3 Bytes address boot mode; 1: 4Bytes address boot mode */
#define GET_FMC_BOOT_MODE ({ \
	unsigned int regval, boot_mode = 0; \
	regval = readl(__io_address(SYS_CTRL_BASE + REG_SC_STAT)); \
	boot_mode = GET_SPI_NOR_ADDR_MODE(regval); \
	boot_mode; })

/****************************************************************************/
void hifmc_set_nor_system_clock(struct spi_op *op, int clk_en)
{
	unsigned int old_val, regval;

	old_val = regval = readl(__io_address(CRG_REG_BASE + REG_FMC_CRG));

	regval &= ~FMC_CLK_SEL_MASK;

	if (op && op->clock)
		regval |= op->clock & FMC_CLK_SEL_MASK;
	else
		regval |= FMC_CLK_SEL(FMC_CLK_24M);	/* Default Clock */

	if (clk_en)
		regval |= FMC_CLK_ENABLE;
	else
		regval &= ~FMC_CLK_ENABLE;

	if (regval != old_val)
		writel(regval, __io_address(CRG_REG_BASE + REG_FMC_CRG));
}

/****************************************************************************/
void hifmc_get_nor_best_clock(unsigned int *clock)
{
	int ix;
	int clk_reg;
	const char *str[] = {"12MHz", "41.65MHz", "62.5MHz", "75MHz"};

#define CLK_2X(_clk)	(((_clk) + 1) >> 1)
	unsigned int sysclk[] = {
		CLK_2X(24),	FMC_CLK_SEL(FMC_CLK_24M),
		CLK_2X(83),	FMC_CLK_SEL(FMC_CLK_83M),
		CLK_2X(125),	FMC_CLK_SEL(FMC_CLK_125M),
		CLK_2X(150),	FMC_CLK_SEL(FMC_CLK_150M),
		0,		0,
	};
#undef CLK_2X

	clk_reg = FMC_CLK_SEL(FMC_CLK_24M);
	FMC_PR(QE_DBG, "\t|*-matching flash clock %d\n", *clock);
	for (ix = 0; sysclk[ix]; ix += 2) {
		if (*clock < sysclk[ix])
			break;
		clk_reg = sysclk[ix + 1];
		FMC_PR(QE_DBG, "\t||-select system clock: %s\n",
				str[clk_reg]);
	}

	FMC_PR(QE_DBG, "\t|*-matched best system clock: %s\n",
			str[clk_reg]);
	*clock = clk_reg;
}
