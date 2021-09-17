/******************************************************************************
 *	Flash Memory Controller v100 Device Driver
 *	Copyright (c) 2014 - 2015 by Hisilicon
 *	All rights reserved.
 * ***
 *	Create by hisilicon
 *
 *****************************************************************************/

/*****************************************************************************/
#define GET_FMC_BOOT_MODE ({ \
	unsigned int reg, boot_mode = 0, base = IO_ADDRESS(SYS_CTRL_BASE); \
	reg = readl((void *)(base + SYS_CTRL_SYSSTAT)); \
	boot_mode = GET_SPI_NOR_ADDRESS_MODE(boot_mode); \
	boot_mode; })

/*****************************************************************************/
void hifmc_set_nor_system_clock(struct spi_op *op, int clk_en)
{
	unsigned int base = IO_ADDRESS(REG_BASE_CRG);
	unsigned int old_val, regval = readl((void *)(base + FMC_CRG29));

	old_val = regval;

	regval &= ~FMC_CLK_SEL_MASK;

	if (op && op->clock)
		regval |= op->clock & FMC_CLK_SEL_MASK;
	else
		regval |= FMC_CLK_SEL_24M;		/* Default Clock */

	if (clk_en)
		regval |= FMC_CRG29_CLK_EN;
	else
		regval &= ~FMC_CRG29_CLK_EN;

	if (regval != old_val)
		writel(regval, (void *)(base + FMC_CRG29));
}

/*****************************************************************************/
void hifmc_get_nor_best_clock(unsigned int *clock)
{
	int ix;
	int clk_reg;

#define CLK_2X(_clk)	(((_clk) + 1) >> 1)
	unsigned int sysclk[] = {
		CLK_2X(24),	FMC_CLK_SEL_24M,
		CLK_2X(83),	FMC_CLK_SEL_83M,
		CLK_2X(150),	FMC_CLK_SEL_150M,
		0,		0,
	};
#undef CLK_2X

	clk_reg = FMC_CLK_SEL_24M;
	for (ix = 0; sysclk[ix]; ix += 2) {
		if (*clock < sysclk[ix])
			break;
		clk_reg = sysclk[ix + 1];
	}

	*clock = clk_reg;
}
