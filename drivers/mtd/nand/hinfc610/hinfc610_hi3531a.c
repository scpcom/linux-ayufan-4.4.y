/****************************************************************************
 *	NAND Flash Controller V610 Device Driver
 *	Copyright (c) 2009 - 2015 by Hisilicon
 *	All rights reserved.
 * ***
 *	Create by hisilicon
 *
 ****************************************************************************/
#include <mach/hardware.h>

/****************************************************************************/
void hinfc610_controller_enable(struct hinfc_host *host, int enable)
{
	unsigned int old_val, regval;

	old_val = regval = readl(__io_address(CRG_REG_BASE + REG_NFC_CRG));

	if (enable)
		regval |= (NFC_CLK_ENABLE | NFC_CLK_SEL(NFC_CLK_200M));
	else
		regval &= ~NFC_CLK_ENABLE;

	if (regval != old_val)
		writel(regval, __io_address(CRG_REG_BASE + REG_NFC_CRG));
}
