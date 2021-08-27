/*
 *  linux/arch/arm/mach-dove/clock.c
 */

/* TODO: Implement the functions below...	*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <mach/hardware.h>

#include "clock.h"

int clk_enable(struct clk *clk)
{
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	return 0;
}
EXPORT_SYMBOL(clk_get_rate);

void clks_register(struct clk *clks, size_t num)
{
}

static int __init clk_init(void)
{
	/* TODO: Call clks_register with appropriate params. */
	clks_register(NULL, 0);
	return 0;
}
arch_initcall(clk_init);
