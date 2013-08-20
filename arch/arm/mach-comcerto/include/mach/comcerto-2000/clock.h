/*
 * arch/arm/mach-comcerto/clock.h
 *
 * Clock control driver for Comcerto C2K device - internal header file
 *
 * This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ARCH_ARM_C2K_CLOCK_H__
#define __ARCH_ARM_C2K_CLOCK_H__

#define CLK_DOMAIN_MASK (1<<0)
#define CLK_DOMAIN_SPI_I2C_MASK (1<<5)
#define CLK_DOMAIN_TDMNTG_MASK (1<<4)
#define CLK_DOMAIN_UART_MASK (1<<6)
#define CLK_DOMAIN_PCIE0_MASK (1<<0)
#define CLK_DOMAIN_PCIE1_MASK (1<<1)
#define CLK_DOMAIN_IPSEC_SPACC_MASK (1<<2)
#define CLK_DOMAIN_DPI_CIE_MASK (1<<5)
#define CLK_DOMAIN_DPI_DECOMP_MASK (1<<6)
#define CLK_DOMAIN_USB0_MASK (1<<3)
#define CLK_DOMAIN_USB1_MASK (1<<4)
#define CLK_DOMAIN_DUS_MASK  (1<<0)
#define CLK_DOMAIN_SATA_MASK (1<<2)

struct clk {
	struct clk *parent;
	const char *name;
	unsigned long  rate;          /* Default rate value in HZ */
	int usecount;
	u32             enable_reg;
	u32             clkgen_reg;
	u32             div_reg;
	u32             enable_mask;
	unsigned long	(*get_rate)(struct clk *);
        int 		(*set_rate) (struct clk *, unsigned long rate);
};
extern int clk_init(void);
#endif
