/*
 *  linux/arch/arm/mach-comcerto/clock.c
 *
 *  Copyright (C) 2008 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <linux/module.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <asm/io.h>
#include <mach/hardware.h>

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clkdev.h>
#include <linux/printk.h>
#include <linux/list.h>
#include <mach/comcerto-2000/clock.h>

static DEFINE_SPINLOCK(clock_lock);

/* Forward declaration */
static void HAL_set_clk_divider(unsigned long ,u32 , u32);
static unsigned long HAL_get_clk_freq(u32, u32);

/* API clk_put clk_get
 * Declared in "include/linux/clk.h", include this
 * in correspoding driver.
 * Defined in drivers/clk/clkdev.c, will
 * use the above.
 */

/* @ struct clk *
 * API:local_clk_disable
 * Call the device level disable.
 * This will decrease the  usecount following to disabling
 * the clock.
*/
static void local_clk_disable(struct clk *clk)
{
	u32 val;
	if (clk->usecount == 0){
		pr_warning("Warning : %s Clock is already disabled \n",clk->name);
		return;
	}

	/* Decrement the usecount */
	clk->usecount--;

	if (!clk->usecount){
		/* Take care of parent disable , if present for the clock.
		 * Disable the parent , if no other clock is using.
		*/
		if (clk->parent)
			local_clk_disable(clk->parent);

		/* Apply the Clock register write here */
		if (clk->enable_reg){
			val = readl(clk->enable_reg);
			val &= ~clk->enable_mask;
			writel(val , clk->enable_reg);
		}
	}
}

/* @ struct clk *
 * API:clk_disable used to disable clock for
 * the devices.
 * This API will be available to outside
 * ( for all device driver).
 * Call of this API should be followed by clk_get
 * clk_disable->clk_put.
*/

void clk_disable(struct clk *clk)
{
        unsigned long flags;

        spin_lock_irqsave(&clock_lock, flags);
        local_clk_disable(clk);
        spin_unlock_irqrestore(&clock_lock, flags);
}

EXPORT_SYMBOL(clk_disable);

/* @ struct clk *
 * API : Internal call for clk_disable_unused for 
 * unused devices.
*/
static void __clk_disable_unused(struct clk *clk)
{
	unsigned long flags;
	u32 val;

        spin_lock_irqsave(&clock_lock, flags);
	/* Apply the Clock register write here */
	if (clk->enable_reg){
		val = readl(clk->enable_reg);
		val &= ~clk->enable_mask;
                writel(val , clk->enable_reg);
	}
        spin_unlock_irqrestore(&clock_lock, flags);
}

/* @ struct clk *
 * API:local_clk_enable
 * Call the device level enable.
 * This will increase the  usecount.
*/
static int local_clk_enable(struct clk *clk)
{
	int ret = 0;
	u32 val;

	if (clk->usecount == 0) {
		/* Check for parent clock .Enable the parent clock,if available*/
		if (clk->parent){
			ret = local_clk_enable(clk->parent);
			if (ret)
				return -EINVAL;
		}
		if (clk->enable_reg){
			val = readl(clk->enable_reg);
                	val |= clk->enable_mask;
                	writel(val , clk->enable_reg);
			ret=0;
		}
	}
	/* Increment the use count */
	clk->usecount++;
        return ret;
}

/* @ struct clk *
 * API:clk_enable used to enable clock for
 * the devices.
 * This API will be available to outside
 * ( for all device driver).
 * Call of this API should be followed by clk_get
 * clk_get -> clk_enable.
*/
int clk_enable(struct clk *clk)
{
	unsigned long flags;
	int ret=0;

	if (!clk)
		return -EINVAL;

	spin_lock_irqsave(&clock_lock, flags);
	ret = local_clk_enable(clk);
	spin_unlock_irqrestore(&clock_lock, flags);

	return ret;
}
EXPORT_SYMBOL(clk_enable);


/* @ struct clk *
 * API:clk_get_rate used to get clock rate value
 * for the devices. This API will be available to
 * outside ( for all device driver).
*/
unsigned long clk_get_rate(struct clk *clk)
{
	if (clk->get_rate)
		return clk->get_rate(clk);
	else
		return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

/* @ struct clk * , unsigned long
 * API:local_set_rate used to set clock rate value
 * for the devices.
*/

static int local_set_rate(struct clk *clk, unsigned long rate)
{
	int ret = -EINVAL;
	if (clk->set_rate) {
		ret = clk->set_rate(clk, rate);
	}
	return ret;
}

/* @ struct clk * , unsigned long
 * API:clk_set_rate used to set clock rate value
 * for the devices. This API will be available to
 * outside ( for all device driver).
*/

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long actual_rate;
	int ret = -EINVAL;
        unsigned long flags;

	spin_lock_irqsave(&clock_lock, flags);
	actual_rate=clk->rate;

	if (actual_rate != rate ){
                ret = local_set_rate(clk, rate);
        }
        else{ /*configured rate is same as desired rate*/
                pr_debug("Current rate value of clock source (%s) is same as desired rate value (%ld)\n",clk->name,clk->rate);
                spin_unlock_irqrestore(&clock_lock, flags);
                return 0;
        }

	if ( ret == -EINVAL ){
		pr_debug("Cannot change clock:(%s) ,not supporting set_rate,set to previous value (%ld)\n",clk->name,clk->rate);
	}else{
		pr_debug("Changed clock:(%s) rate ,rate value is (%ld)\n",clk->name,clk->rate);
	}
	spin_unlock_irqrestore(&clock_lock, flags);
	return ret;
}
EXPORT_SYMBOL(clk_set_rate);

/* @ struct clk * 
 * API:clk_get_parent used to parent clock source.
 * This API will be available to
 * outside ( for all device driver).
*/
struct clk *clk_get_parent(struct clk *clk)
{
        return clk->parent;
}
EXPORT_SYMBOL(clk_get_parent);

/* @ struct clk * , unsigned long
 * API:clk_set_parent used to set to a different
 * parent clock source.
 * This API will be available to
 * outside ( for all device driver).
*/

int clk_set_parent(struct clk *clk, struct clk *parent)
{
        unsigned long flags;

	if (clk == NULL || IS_ERR(clk))
                return -EINVAL;

	/* Only set parent once and if none is already set. */
        if (clk->parent)
                return -EINVAL;

	/* Cannot change parent on enabled clock */
        if ( WARN_ON(clk->usecount))
                return -EBUSY;

	spin_lock_irqsave(&clock_lock, flags);
	/* Set the parent in clk  strusture */
        clk->parent = parent;
	spin_unlock_irqrestore(&clock_lock, flags);
        return 0;
}
EXPORT_SYMBOL(clk_set_parent);

/*
 * CLKGEN Divider registers
 * The divider bypass bit in several configuration registers
 * can only be written (if you read back you get zero).
 *
 * The Bug is in registers: 0x84 (A9DP_CLKDIV_CNTRL), 0x84 + 16 (L2CC_CLKDIV_CNTRL), 0x84 + 32
 *	(TPI_CLKDIV_CNTRL), etc... until PLL_ADDR_SPACE at 0x1C0.
 */

/*
 * Barebox uses IRAM to mirror the clock divider registers
 * Linux will relocate this mirror from IRAM to DDR to free up IRAM.
 */
#define IRAM_CLK_REG_MIRROR		(0x8300FC00 - COMCERTO_AXI_IRAM_BASE + IRAM_MEMORY_VADDR)
#define CLK_REG_DIV_BUG_BASE		AXI_CLK_DIV_CNTRL
#define CLK_REG_DIV_BUG_SIZE		(PLL0_M_LSB - AXI_CLK_DIV_CNTRL)

static u8 clk_div_backup_table [CLK_REG_DIV_BUG_SIZE];
#define read_clk_div_bypass_backup(reg) readl(reg - CLK_REG_DIV_BUG_BASE + clk_div_backup_table)
#define write_clk_div_bypass_backup(val, reg) writel(val, reg - CLK_REG_DIV_BUG_BASE + clk_div_backup_table)

void HAL_clk_div_backup_relocate_table (void)
{
	memcpy (clk_div_backup_table, (void*) IRAM_CLK_REG_MIRROR, CLK_REG_DIV_BUG_SIZE);
}
EXPORT_SYMBOL(HAL_clk_div_backup_relocate_table);

/*
 * Get the reference clock after reading bootstrap
 */
unsigned long HAL_get_ref_clk (void)
{
	unsigned long clock_freq = 0;
	unsigned int boot_strap, tmp;

	boot_strap = readl(COMCERTO_GPIO_SYSTEM_CONFIG);
	tmp = (boot_strap & GPIO_SYS_PLL_REF_CLK_MASK) >> GPIO_SYS_PLL_REF_CLK_SHIFT;

	if ( USB_XTAL_REF_CLK == tmp )
	{
			if ( boot_strap & GPIO_USB_OSC_PAD_MASK )
				clock_freq = REF_CLK_24MHZ;
			else
				clock_freq = REF_CLK_48MHZ;
	}
	else if ( SERDES_XTAL_REF_CLK == tmp )
	{
		if ( boot_strap & GPIO_SERDES_OSC_PAD_MASK )
			clock_freq = REF_CLK_24MHZ;
		else
			clock_freq = REF_CLK_48MHZ;
	}

	return clock_freq;
}
EXPORT_SYMBOL(HAL_get_ref_clk);

static unsigned long HAL_get_pll_freq(int pll_no)
{
	u32 p;
	u32 od;
	u32 m;
	u32 k;
	u32 s;
	unsigned long pll_clk = 0;
	unsigned long ref_clk = HAL_get_ref_clk();
	unsigned long pll_div = 0;

	if (pll_no < C2K_CLK_PLL3)
	{
		//get NF, NR and OD values
		switch (pll_no)
		{
		case C2K_CLK_PLL0:
			m = readl(PLL0_M_LSB) & 0xff;
			m |= (readl(PLL0_M_MSB) & 0x3) << 8;
			p = readl(PLL0_P) & 0x3f;
			s = readl(PLL0_S) & 0x7;
			od = (1 << s); // 2^s;
			pll_div = readl(PLL0_DIV_CNTRL);
			break;

		case C2K_CLK_PLL1:
			m = readl(PLL1_M_LSB) & 0xff;
			m |= (readl(PLL1_M_MSB) & 0x3) << 8;
			p = readl(PLL1_P) & 0x3f;
			s = readl(PLL1_S) & 0x7;
			od = (1 << s);
			pll_div = readl(PLL1_DIV_CNTRL);
			break;

		case C2K_CLK_PLL2:
			m = readl(PLL2_M_LSB) & 0xff;
			m |= (readl(PLL2_M_MSB) & 0x3) << 8;
			p = readl(PLL2_P) & 0x3f;
			s = readl(PLL2_S) & 0x7;
			od = (1 << s);
			pll_div = readl(PLL2_DIV_CNTRL);
			break;

		default:
			return 0;
			break;
		}

		/* Ref Clock divided by 1000000. It should be displayed in MHz. */
		pll_clk = ((ref_clk / 1000000) * m) / p / od ;
		
		if (pll_div & CLK_DIV_BYPASS)
			pll_div = 1;
		else
			pll_div = pll_div & 0x1F;

		if (pll_div > 1)
			pll_clk = pll_clk/pll_div;

	}
	else if (pll_no == C2K_CLK_PLL3)
	{
		m = readl(PLL3_M_LSB) & 0xff;
		m |= (readl(PLL3_M_MSB) & 0x3) << 8;
		p = readl(PLL3_P) & 0x3f;
		s = readl(PLL3_S) & 0x7;
		k = readl(PLL3_K_LSB) & 0xff;
		k |= (readl(PLL3_K_MSB) & 0xf) << 8;
		od = (1 << s);
		pll_clk = (((ref_clk / 1000000) * (m * 1024 + k)) / p / od + 1023) / 1024;
	}

	return (pll_clk * 1000000); /* convert into Hz and return it */
}

static void HAL_set_clk_divider(unsigned long rate,u32 ctrl_reg, u32 div_reg)
{
	u32 pll_src;
	u32 val;
	int divider;
	unsigned long  pll_rate;

	/* Get PLL Source */
	pll_src = readl(ctrl_reg);
	pll_src = (pll_src >> CLK_PLL_SRC_SHIFT) & CLK_PLL_SRC_MASK;

	/* Get PLL Freq */
	pll_rate = HAL_get_pll_freq(pll_src);

	/* Get The Divider value For  Clock */
	divider   =  pll_rate/rate;

	if ( divider == 1){
		write_clk_div_bypass_backup(CLK_DIV_BYPASS,div_reg);
		/* Enable the Bypass bit in Hw reg (clk_div_bypass in div_reg) */
		val = readl(div_reg);
		val |= CLK_DIV_BYPASS;
		writel(val , div_reg);
	}
	else
	{
		write_clk_div_bypass_backup(0,div_reg);
		/* Write to the divider reg */
		val = readl(div_reg);
		val &= ~0x1f;
		val |= divider;
		writel(val, div_reg);
		/* Clear the Bypass bit in Hw reg (clk_div_bypass in div_reg) */
		val = readl(div_reg);
		val &= ~CLK_DIV_BYPASS;
		writel(val , div_reg);
	}
}

static int HAL_get_clock_pll_source(u32 ctrl_reg)
{
	int pll_src;

	/* Get PLL source */
	pll_src = readl(ctrl_reg);
	pll_src = (pll_src >> CLK_PLL_SRC_SHIFT) & CLK_PLL_SRC_MASK;

	return pll_src;

}

static void HAL_set_clock_pll_source(u32 ctrl_reg,int pll_src){

	switch(pll_src)
	{
		case C2K_CLK_PLL0:
			writel(readl(ctrl_reg) | (1 << 0) , ctrl_reg);
                	break;
		case C2K_CLK_PLL1:
			writel(readl(ctrl_reg) | (1 << 1) , ctrl_reg);
                	break;
		case C2K_CLK_PLL3:
			writel(readl(ctrl_reg) | (1 << 3), ctrl_reg);
                	break;
		case C2K_CLK_PLL2:
		default:
			writel(readl(ctrl_reg) | (1 << 2), ctrl_reg);
                	break;
	}

}

static unsigned long HAL_get_clk_freq(u32 ctrl_reg, u32 div_reg)
{
	u32 pll_src;
	u32 clk_div;
	unsigned long clk_out;
	unsigned long pll_clk;
	int bypass = 0;

	/* Get PLL Source */
	pll_src = readl(ctrl_reg);
	pll_src = (pll_src >> CLK_PLL_SRC_SHIFT) & CLK_PLL_SRC_MASK;

	/* Get clock divider bypass value from IRAM Clock Divider registers mirror location */
	clk_div = read_clk_div_bypass_backup(div_reg);

	if (clk_div & CLK_DIV_BYPASS)
		bypass = 1;
	else
	{
		clk_div = readl(div_reg);
		clk_div &= 0x1f;
	}

	pll_clk = HAL_get_pll_freq(pll_src); 

	if (bypass)
		clk_out = pll_clk;
	else
		clk_out = pll_clk / clk_div;

	return clk_out;
}

/* Get rate API callback */
static unsigned long HAL_get_clk(struct clk *clk)
{
	return HAL_get_clk_freq(clk->clkgen_reg, clk->div_reg);
}

/* Get rate API callback for those clocks having Parent */
static unsigned long HAL_get_parent_clk(struct clk *clk)
{
	return clk_get_rate(clk->parent);
}
/* Specific Get rate API callback for arm peripherals */
static unsigned long  HAL_get_arm_peri_clk(struct clk *clk)
{
	return clk_get_rate(clk->parent)/4 ;
}

/* Set rate APIs callbacks */
static int HAL_set_clk(struct clk *clk,unsigned long rate)
{
	struct clk *clk_parent;
	/* Get the parent clock */
	clk_parent=clk_get_parent(clk);
	if ( clk_parent || rate <= clk_parent->rate){
		/* Set the divider value to reg */
		HAL_set_clk_divider(rate,clk->clkgen_reg,clk->div_reg);
		/* Set the rate value to clk structure */
		clk->rate=rate;
		return 0;
	}else
		return -EINVAL;
}

/* Initization of clk structure for individual device */

static struct clk clk_pll0 = {
	.name     = "pll0",
};

static struct clk clk_pll1 = {
	.name   = "pll1",
};

static struct clk clk_pll2 = {
	.name   = "pll2",
};

static struct clk clk_pll3 = {
	.name   = "pll3",
};



static struct clk gemtx_clk = {
	.name        = "gemtx",
	.enable_reg  = GEMTX_CLK_CNTRL,
	.clkgen_reg  = GEMTX_CLK_CNTRL,
	.div_reg     = GEMTX_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk ddr_clk = {
	.name        = "ddr",
	.enable_reg  = DDR_CLK_CNTRL,
	.clkgen_reg  = DDR_CLK_CNTRL,
	.div_reg     = DDR_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
};

static struct clk arm_clk = {
	.name        = "arm",
	.enable_reg  = A9DP_CLK_CNTRL,
	.clkgen_reg  = A9DP_CLK_CNTRL,
	.div_reg     = A9DP_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk l2cc_clk = {
	.name        = "l2cc",
	.enable_reg  = L2CC_CLK_CNTRL,
	.clkgen_reg  = L2CC_CLK_CNTRL,
	.div_reg     = L2CC_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk axi_clk = {
	.name        = "axi",
	.enable_reg  = AXI_CLK_CNTRL_0,
	.clkgen_reg  = AXI_CLK_CNTRL_0,
	.div_reg     = AXI_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk uart_clk = { /* Legacy UART */
	.parent      = &axi_clk,
	.name        = "uart",
	.enable_reg  = AXI_CLK_CNTRL_1,
	.enable_mask = CLK_DOMAIN_UART_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk ipsec_eape_clk = {
	.name        = "ipsec_eape",
	.enable_reg  = IPSEC_CLK_CNTRL,
	.clkgen_reg  = IPSEC_CLK_CNTRL,
	.div_reg     = IPSEC_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
};

static struct clk ipsec_spacc_clk = {       /* IPSEC spacc clock for Elliptic EPN1802*/
	.parent      = &axi_clk,
	.name        = "ipsec_spacc",
	.enable_reg  = AXI_CLK_CNTRL_1,
	.enable_mask = CLK_DOMAIN_IPSEC_SPACC_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk dpi_cie_clk = {       /* DPI cie clock*/
	.parent      = &axi_clk,
	.name        = "dpi_cie",
	.enable_reg  = AXI_CLK_CNTRL_0,
	.enable_mask = CLK_DOMAIN_DPI_CIE_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk dpi_decomp_clk = {       /* DPI decomp clock*/
	.parent      = &axi_clk,
	.name        = "dpi_decomp",
	.enable_reg  = AXI_CLK_CNTRL_0,
	.enable_mask = CLK_DOMAIN_DPI_DECOMP_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk DUS_clk = { /* DMA,FAST-UART and SMI clock */
	.parent      = &axi_clk,
	.name        = "DUS",
	.enable_reg  = AXI_CLK_CNTRL_1,
	.enable_mask = CLK_DOMAIN_DUS_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk arm_peri_clk = {
	.parent      = &arm_clk,
	.name        = "arm_peri",
	.enable_reg  = A9DP_MPU_CLK_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_arm_peri_clk,
};

static struct clk hfe_core_clk = {
	.name        = "hfe_core",
	.enable_reg  = PFE_CLK_CNTRL,
	.clkgen_reg  = PFE_CLK_CNTRL,
	.div_reg     = PFE_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk spi_i2c_clk = {
	.parent      = &axi_clk,
	.name        = "spi_i2c",
	.enable_reg  = AXI_CLK_CNTRL_1,
	.enable_mask = CLK_DOMAIN_SPI_I2C_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk tdmNTG_clk = {
	.name        = "ntgref",
	.enable_reg  = TDMNTG_REF_CLK_CNTRL,
	.clkgen_reg  = TDMNTG_REF_CLK_CNTRL,
	.div_reg     = TDMNTG_REF_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk pcie0_clk = {
	.parent      = &axi_clk,
	.name        = "pcie0",
	.enable_reg  = AXI_CLK_CNTRL_2,
	.enable_mask = CLK_DOMAIN_PCIE0_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk pcie1_clk = {
	.parent      = &axi_clk,
	.name        = "pcie1",
	.enable_reg  = AXI_CLK_CNTRL_2,
	.enable_mask = CLK_DOMAIN_PCIE1_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk usb0_clk = {
	.parent      = &axi_clk,
	.name        = "usb0",
	.enable_reg  = AXI_CLK_CNTRL_2,
	.enable_mask = CLK_DOMAIN_USB0_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk usb1_clk = {
	.parent      = &axi_clk,
	.name        = "usb1",
	.enable_reg  = AXI_CLK_CNTRL_2,
	.enable_mask = CLK_DOMAIN_USB1_MASK,
	.get_rate    = HAL_get_parent_clk,
};

static struct clk sata_clk = {            /* SATA AXI clock */
	.parent      = &axi_clk,
	.name        = "sata",
	.enable_reg  = AXI_CLK_CNTRL_2,
	.enable_mask = CLK_DOMAIN_SATA_MASK,
	.get_rate    = HAL_get_parent_clk,
};
static struct clk sata_oob_clk = {
	.name        = "sata_oob",	  /* SATA PMU alive clock */
	.enable_reg  = SATA_OOB_CLK_CNTRL,
	.clkgen_reg  = SATA_OOB_CLK_CNTRL,
	.div_reg     = SATA_OOB_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk sata_pmu_clk = {
	.name        = "sata_pmu",	 /* SATA core clock control */
	.enable_reg  = SATA_PMU_CLK_CNTRL,
	.clkgen_reg  = SATA_PMU_CLK_CNTRL,
	.div_reg     = SATA_PMU_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk ext_phy0_clk = {
	.name	     = "ext_phy0",
	.enable_reg  = EXTPHY0_CLK_CNTRL,
	.clkgen_reg  = EXTPHY0_CLK_CNTRL,
	.div_reg     = EXTPHY0_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk ext_phy1_clk = {
	.name	     = "ext_phy1",
	.enable_reg  = EXTPHY1_CLK_CNTRL,
	.clkgen_reg  = EXTPHY1_CLK_CNTRL,
	.div_reg     = EXTPHY1_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk ext_phy2_clk = {
	.name	     = "ext_phy2",
	.enable_reg  = EXTPHY2_CLK_CNTRL,
	.clkgen_reg  = EXTPHY2_CLK_CNTRL,
	.div_reg     = EXTPHY2_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk tpi_clk = {
	.name	     = "tpi",
	.enable_reg  = TPI_CLK_CNTRL,
	.clkgen_reg  = TPI_CLK_CNTRL,
	.div_reg     = TPI_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk csys_clk = {
	.name	     = "csys",
	.enable_reg  = CSYS_CLK_CNTRL,
	.clkgen_reg  = CSYS_CLK_CNTRL,
	.div_reg     = CSYS_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk tsuntg_clk = {
	.name	     = "tsuntg",
	.enable_reg  = TSUNTG_REF_CLK_CNTRL,
	.clkgen_reg  = TSUNTG_REF_CLK_CNTRL,
	.div_reg     = TSUNTG_REF_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk sata_occ_clk = {
	.name	     = "sata_occ",
	.enable_reg  = SATA_OCC_CLK_CNTRL,
	.clkgen_reg  = SATA_OCC_CLK_CNTRL,
	.div_reg     = SATA_OCC_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk pcie_occ_clk = {
	.name	     = "pcie_occ",
	.enable_reg  = PCIE_OCC_CLK_CNTRL,
	.clkgen_reg  = PCIE_OCC_CLK_CNTRL,
	.div_reg     = PCIE_OCC_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

static struct clk sgmii_occ_clk = {
	.name	     = "sgmii_occ",
	.enable_reg  = SGMII_OCC_CLK_CNTRL,
	.clkgen_reg  = SGMII_OCC_CLK_CNTRL,
	.div_reg     = SGMII_OCC_CLK_DIV_CNTRL,
	.enable_mask = CLK_DOMAIN_MASK,
	.get_rate    = HAL_get_clk,
	.set_rate    = HAL_set_clk,
};

/* These clocks are visible outside this module
 * and can be initialized , this list could be expanded
 * according to new device support.
 */
static struct clk *c2k_clks[] __initdata = {
	&clk_pll0,
	&clk_pll1,
	&clk_pll2,
	&clk_pll3,
	&uart_clk,
	&DUS_clk,
	&gemtx_clk,
	&ipsec_eape_clk,
	&ipsec_spacc_clk,
	&dpi_cie_clk,
	&dpi_decomp_clk,
	&ddr_clk,
	&arm_clk,
	&l2cc_clk,
	&axi_clk,
	&arm_peri_clk,
	&hfe_core_clk,
	&spi_i2c_clk,
	&tdmNTG_clk,
	&pcie0_clk,
	&pcie1_clk,
	&usb0_clk,
	&usb1_clk,
	&sata_clk,
	&sata_oob_clk,
	&sata_pmu_clk,
	&ext_phy0_clk,
	&ext_phy1_clk,
	&ext_phy2_clk,
	&tpi_clk,
	&csys_clk,
	&tsuntg_clk,
	&sata_occ_clk,
	&pcie_occ_clk,
	&sgmii_occ_clk,
};

static struct clk_lookup c2k_clksreg[] = {
	{ .clk = &clk_pll0,         .con_id = "pll0"},
	{ .clk = &clk_pll1,         .con_id = "pll1"},
	{ .clk = &clk_pll2,         .con_id = "pll2"},
	{ .clk = &clk_pll3,         .con_id = "pll3"},
	{ .clk = &uart_clk,         .con_id = "uart"},
	{ .clk = &DUS_clk,          .con_id = "DUS"},
	{ .clk = &gemtx_clk,        .con_id = "gemtx"},
	{ .clk = &ipsec_eape_clk,   .con_id = "ipsec_eape"},
	{ .clk = &ipsec_spacc_clk,  .con_id = "ipsec_spacc"},
	{ .clk = &dpi_cie_clk,      .con_id = "dpi_cie"},
	{ .clk = &dpi_decomp_clk,   .con_id = "dpi_decomp"},
	{ .clk = &ddr_clk,          .con_id = "ddr"},
	{ .clk = &arm_clk,          .con_id = "arm"},
	{ .clk = &l2cc_clk,         .con_id = "l2cc"},
	{ .clk = &axi_clk,          .con_id = "axi"},
	{ .clk = &arm_peri_clk,     .con_id = "arm_peri"},
	{ .clk = &hfe_core_clk,     .con_id = "hfe_core"},
	{ .clk = &spi_i2c_clk,      .con_id = "spi_i2c"},
	{ .clk = &tdmNTG_clk,       .con_id = "ntgref"},
	{ .clk = &pcie0_clk,        .con_id = "pcie0"},
	{ .clk = &pcie1_clk,        .con_id = "pcie1"},
	{ .clk = &usb0_clk,         .con_id = "usb0"},
	{ .clk = &usb1_clk,         .con_id = "usb1"},
	{ .clk = &sata_clk,    	    .con_id = "sata"},
	{ .clk = &sata_oob_clk,     .con_id = "sata_oob"},
	{ .clk = &sata_pmu_clk,     .con_id = "sata_pmu"},
	{ .clk = &ext_phy0_clk,     .con_id = "ext_phy0"},
	{ .clk = &ext_phy1_clk,     .con_id = "ext_phy1"},
	{ .clk = &ext_phy2_clk,     .con_id = "ext_phy2"},
	{ .clk = &tpi_clk,     	    .con_id = "tpi"},
	{ .clk = &csys_clk,         .con_id = "csys"},
	{ .clk = &tsuntg_clk,       .con_id = "tsuntg"},
	{ .clk = &sata_occ_clk,     .con_id = "sata_occ"},
	{ .clk = &pcie_occ_clk,     .con_id = "pcie_occ"},
	{ .clk = &sgmii_occ_clk,    .con_id = "sgmii_occ"},
};

/* Initilize all the available clocks */
int clk_init(void){
        struct clk **clkp;
	int pll_no;

	spin_lock_init(&clock_lock);

        /* Determine the barebox configured pll0,pll1,pll2,pll3 rate value */
        clk_pll0.rate = HAL_get_pll_freq(C2K_CLK_PLL0);
        clk_pll1.rate = HAL_get_pll_freq(C2K_CLK_PLL1);
        clk_pll2.rate = HAL_get_pll_freq(C2K_CLK_PLL2);
        clk_pll3.rate = HAL_get_pll_freq(C2K_CLK_PLL3);

	/* Set the NTG ref clock to PLL src (gemtx PLL source)
	 * Currently it is not set from barebox,set here.
	*/
	pll_no = HAL_get_clock_pll_source(gemtx_clk.clkgen_reg);
	HAL_set_clock_pll_source(tdmNTG_clk.clkgen_reg,pll_no);

        pr_info("PLL0 running at %ld MHz, PLL1 at %ld MHz PLL2 running at %ld MHz PLL3 running at %ld MHz\n",
                clk_pll0.rate/1000000, clk_pll1.rate/1000000, clk_pll2.rate/1000000, clk_pll3.rate/1000000);
        /* Initilization of Clocks present in C2k device */
        for (clkp = c2k_clks; clkp < c2k_clks + ARRAY_SIZE(c2k_clks);clkp++) {
                struct clk *clk = *clkp;
		/* Setting the parent clock in the init
		 * Only to those who can be configured with PLL source
		 */
		if (!clk->parent){
			if (clk->clkgen_reg){ /* This check is for not to set parent for PLLS */
				pll_no = HAL_get_clock_pll_source(clk->clkgen_reg);
				switch (pll_no)
				{
					case C2K_CLK_PLL0:
						clk_set_parent(clk,&clk_pll0);
						break;
					case C2K_CLK_PLL1:
						clk_set_parent(clk,&clk_pll1);
						break;
					case C2K_CLK_PLL2:
						clk_set_parent(clk,&clk_pll2);
						break;
					default:
						break;
				}
			}
		}
        	/* Get The Device Clock Rate Values */
		if ( clk->get_rate)
			clk->rate=clk->get_rate(clk);
                pr_debug("%s: clock %s, rate %ld\n",__func__, clk->name, clk->rate);
        }

        /* Creation of Clock Device Tree */
        clkdev_add_table(c2k_clksreg, ARRAY_SIZE(c2k_clksreg));
	return 0;

}
EXPORT_SYMBOL(clk_init);

/*
 * Disable any unused clocks left on by the bootloader(Barebox)
 */
static int __init clk_disable_unused(void)
{
        struct clk **clkp;

        for (clkp = c2k_clks; clkp < c2k_clks + ARRAY_SIZE(c2k_clks);clkp++) {
                struct clk *clk = *clkp;
		if (clk->usecount > 0)
			continue;

		/* FIXME Currently there is no clock FW support for the
                 * following clocks in corresponding driver . So skipping these
		 * clocks from by default disable state.
                 * All other unused clocks will be disabled.
                */
		if ( strcmp(clk->name, "hfe_core") &&
				strcmp(clk->name, "ipsec_eape") &&
				strcmp(clk->name, "pll3") &&
				strcmp(clk->name, "arm_peri") )
		{
			pr_info("Clocks: disabled unused %s\n", clk->name);
			__clk_disable_unused(clk);
		}
	}
        return 0;
}
late_initcall(clk_disable_unused);
