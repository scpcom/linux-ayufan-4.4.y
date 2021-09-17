#include <linux/io.h>
#include <linux/platform_device.h>
#include "tnk_hi3536.h"
#include "tnkhw.h"

#undef TNK_PRINT

unsigned long tnk_clk_init(void)
{
	unsigned long tnkclk;
	unsigned long tnkctl;
	unsigned long toe_clk_srst;
	void __iomem *toe_clk_reg;

	toe_clk_reg = (void __iomem *)(REG_CRG0_OFFSET + TOE_CLK_SRST);

	tnkclk = mdio_clk_init();

	toe_clk_srst = readl(toe_clk_reg);
#ifdef TNK_PRINT
	pr_info("%s: tnkclk: %.8x, toe_clk_srst: %.8x\n",
	       __func__, (unsigned int)tnkclk, (unsigned int)toe_clk_srst);
#endif

	tnkctl = ((tnkclk / 1000) << 12) | TOE_MAX_RETRY_NUM;
#ifdef TNK_PRINT
	pr_info
		("%s: TOE_DEFAULT_CLK %x, TOE_MAX_RETRY_NUM %x,tnkctl %x\n",
		 __func__, tnkclk, TOE_MAX_RETRY_NUM,
		 (unsigned int)tnkctl);
#endif
	return tnkctl;
}

unsigned long mdio_clk_init(void)
{
	unsigned long toe_clk_val;
	void __iomem *toe_clk_reg;

	toe_clk_reg = (void __iomem *)(REG_CRG0_OFFSET + TOE_CLK_SRST);
	toe_clk_val = readl(toe_clk_reg);

	if (toe_clk_val & TOE_CLK_DEF_250M)
		return TOE_DEFAULT_CLK_250M;
	else if (!(toe_clk_val & TOE_CLK_DEF_250M))
		return TOE_DEFAULT_CLK_150M;
	else
		return 0;
}

int get_clk_csr(unsigned long tnkclk)
{
	int clk_csr;

	if ((tnkclk >= 60000000) && (tnkclk <= 100000000))
		clk_csr = 0;
	else if ((tnkclk >= 100000000) && (tnkclk <= 150000000))
		clk_csr = 1;
	else if ((tnkclk >= 20000000) && (tnkclk <= 35000000))
		clk_csr = 2;
	else if ((tnkclk >= 35000000) && (tnkclk <= 60000000))
		clk_csr = 3;
	else if ((tnkclk >= 150000000) && (tnkclk <= 250000000))
		clk_csr = 4;
	else if ((tnkclk >= 250000000) && (tnkclk <= 300000000))
		clk_csr = 5;
	else
		clk_csr = -1;

	/* HI3536 TOE use clk 150MHz or 250MHz, so always set clk_csr to 4 */
	clk_csr = 4;

	return clk_csr;
}

void reset_mac_interface_dual(int id, void *addr, u32 val)
{
	unsigned int reg;

	/*  Enter reset mode */
	reg = readl(addr + TOE_CLK_SRST);
	reg |= (0x10 << (id << 1));
	writel(reg, addr + TOE_CLK_SRST);
	/*  Write the new configuration value */
	writel(val, addr + TOE_MAC_INTERFACE);
	/*  Make sure the write is fully flushed */
	readl(addr + TOE_MAC_INTERFACE);
	/*  Leave reset mode */
	reg &= ~(0x10 << (id << 1));
	writel(reg, addr + TOE_CLK_SRST);
}

void stmmac_rst_phy_use_crg(int port_id, void *syscfg_addr)
{
	unsigned int reg;
	unsigned int phy_rst_bit;

	phy_rst_bit = TOE_CLK_EXT_PHY1_RST_BIT;

	/* Leave reset mode, make sure the RESET has a negative pulse */
	reg = readl(syscfg_addr + TOE_CLK_SRST);
	reg &= ~(1 << phy_rst_bit);
	writel(reg, syscfg_addr + TOE_CLK_SRST);

	mdelay(30);

	/*  Enter reset mode */
	reg = readl(syscfg_addr + TOE_CLK_SRST);
	reg |= (1 << phy_rst_bit);
	writel(reg, syscfg_addr + TOE_CLK_SRST);

	mdelay(30);

	/*  Leave reset mode */
	reg = readl(syscfg_addr + TOE_CLK_SRST);
	reg &= ~(1 << phy_rst_bit);
	writel(reg, syscfg_addr + TOE_CLK_SRST);

	/* wait at least 30ms for future MDIO operation */
	mdelay(30);
}

void stmmac_rst_phy_use_gpio(int port_id)
{
	unsigned int gpio_base = 0;
	unsigned int gpio_bit;
	unsigned int rst_data;
	unsigned int v;

	if (port_id == TNK_GMAC0_ID) {
#ifdef CONFIG_MAC0_PHY_GPIO_RESET
		gpio_base = CONFIG_MAC0_PHY_RESET_GPIO_BASE;
		gpio_bit = CONFIG_MAC0_PHY_RESET_GPIO_BIT;
		rst_data = CONFIG_MAC0_PHY_RESET_GPIO_DATA;
#endif
	} else {
#ifdef CONFIG_MAC1_PHY_GPIO_RESET
		gpio_base = CONFIG_MAC1_PHY_RESET_GPIO_BASE;
		gpio_bit = CONFIG_MAC1_PHY_RESET_GPIO_BIT;
		rst_data = CONFIG_MAC1_PHY_RESET_GPIO_DATA;
#endif
	}

#if defined(CONFIG_MAC0_PHY_GPIO_RESET) || defined(CONFIG_MAC0_PHY_GPIO_RESET)
	if (!gpio_base)
		return;

	gpio_base = IO_ADDRESS(gpio_base);

	/* config gpip[x] dir to output */
	v = readb((void __iomem *)(gpio_base + REG_GPIO_DIR));
	v |= (1 << gpio_bit);
	writeb(v, (void __iomem *)(gpio_base + REG_GPIO_DIR));

	/* gpiox[x] set to reset, then delay 30ms */
	writeb(rst_data << gpio_bit,
			(void __iomem *)(gpio_base + (4 << gpio_bit)));
	mdelay(30);
	/* then,cancel reset,and should delay 30ms */
	writeb((!rst_data) << gpio_bit,
			(void __iomem *)(gpio_base + (4 << gpio_bit)));
	mdelay(30);
	writeb(rst_data << gpio_bit,
			(void __iomem *)(gpio_base + (4 << gpio_bit)));

	/* wait at least 30ms for future MDIO operation */
	mdelay(30);
#endif
}

#define EXT_PHY_RST_USE_GPIO	(0)
#define EXT_PHY_RST_USE_CRG	(1)
void stmmac_external_phy_reset(int port_id, void *syscfg_addr)
{
	unsigned int how_to_rst;

	/* reset phy0 will reset phy1 at the same time,
	 * so we no need to reset phy1 any more.
	 */
	if (port_id != TNK_GMAC0_ID)
		return;

	how_to_rst = readl((void __iomem *)
			(MUXCTL_BASE + MUXCTL_REG34_OFFSET));

	how_to_rst &= MUXCTL_PHY_MASK;

	if (how_to_rst == EXT_PHY_RST_USE_CRG)
		stmmac_rst_phy_use_crg(port_id, syscfg_addr);
	else
		stmmac_rst_phy_use_gpio(port_id);
}
