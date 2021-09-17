#ifdef CONFIG_ARCH_HI3521A

#include "hi_ahci_sys_hi3521a_defconfig.h"

enum {
	HI_SATA_PERI_CTRL		= IO_ADDRESS(0x12040000),
	HI_SATA_PERI_CRG26		= (HI_SATA_PERI_CTRL + 0x68),
	HI_SATA_CLK_VALUE		= 0x5,

	HI_SATA_PERI_CRG27		= (HI_SATA_PERI_CTRL + 0x6C),
	HI_SATA_PORT1_REFCLK_CKEN	= (1 << 17),
	HI_SATA_PORT1_MPLL_CKEN		= (1 << 16),
	HI_SATA_CK0_RESET		= (1 << 13),
	HI_SATA_BUS_RESET		= (1 << 12),
	HI_SATA_PORT0_REFCLK_CKEN	= (1 << 10),
	HI_SATA_PORT0_MPLL_CKEN		= (1 << 9),
	HI_SATA_CKO_ALIVE_CKEN		= (1 << 8),
	HI_SATA_RX1_CKEN		= (1 << 6),
	HI_SATA_RX0_CKEN		= (1 << 5),
	HI_SATA_BUS_CKEN		= (1 << 4),
	HI_SATA_PORT01_CLK_EN           = HI_SATA_BUS_CKEN
					| HI_SATA_RX0_CKEN
					| HI_SATA_RX1_CKEN
					| HI_SATA_CKO_ALIVE_CKEN
					| HI_SATA_PORT0_MPLL_CKEN
					| HI_SATA_PORT0_REFCLK_CKEN
					| HI_SATA_PORT1_MPLL_CKEN
					| HI_SATA_PORT1_REFCLK_CKEN,

};

static void hi_sata_poweron(void)
{
}

static void hi_sata_poweroff(void)
{
}

static void hi_sata_reset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG27);
	tmp_val |= HI_SATA_BUS_RESET;
	tmp_val |= HI_SATA_CK0_RESET;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG27);
}

static void hi_sata_unreset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG27);
	tmp_val &= ~(HI_SATA_BUS_RESET | HI_SATA_CK0_RESET);
	writel(tmp_val, (void *)HI_SATA_PERI_CRG27);
}

static void hi_sata_phy_reset(void)
{
}

static void hi_sata_phy_unreset(void)
{
}

static void hi_sata_clk_enable(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG27);
	tmp_val |= HI_SATA_PORT01_CLK_EN;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG27);
}

static void hi_sata_clk_disable(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG27);
	tmp_val &= ~HI_SATA_PORT01_CLK_EN;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG27);
}

static void hi_sata_clk_reset(void)
{
}

static void hi_sata_clk_unreset(void)
{
}

static void hi_sata_phy_clk_sel(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG26);
	tmp_val |= HI_SATA_CLK_VALUE;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG26);
}

void hisata_v200_set_fifo(void *mmio, int n_ports)
{
	int i;

	for (i = 0; i < n_ports; i++)
		writel(HI_SATA_FIFOTH_VALUE, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_FIFOTH));
}

void hisata_phy_init(void *mmio, int phy_mode, int n_ports)
{
	unsigned int tmp, phy_config;
	int i;

	hisata_v200_set_fifo(mmio, n_ports);

	if ((n_ports < 1) || (n_ports > 2))
		pr_err("ERROR: PORT num you set is WRONG!!!\n");

	writel(HI_SATA_PHY_VALUE, (mmio + HI_SATA_PHY0_CTLL));
	writel(HI_SATA_PHY_VALUE, (mmio + HI_SATA_PHY1_CTLL));

	for (i = 0; i < n_ports; i++)
		writel(HI_SATA_PHYCTL2_VALUE, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL2));

	tmp = readl(mmio + HI_SATA_PHY0_CTLL);
	tmp |= HI_SATA_PHY_REV_CLK;
	writel(tmp, (mmio + HI_SATA_PHY0_CTLL));
	tmp = readl(mmio + HI_SATA_PHY1_CTLL);
	tmp |= HI_SATA_PHY_REV_CLK;
	writel(tmp, (mmio + HI_SATA_PHY1_CTLL));

	for (i = 0; i < n_ports; i++) {
		tmp = readl(mmio + 0x100 + i*0x80
				+ HI_SATA_PORT_PHYCTL2);
		tmp &= ~HI_SATA_LANE0_RESET;
		writel(tmp, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL2));
	}

	tmp = readl(mmio + HI_SATA_PHY0_CTLL);
	tmp &= ~HI_SATA_PHY_RESET;
	writel(tmp, (mmio + HI_SATA_PHY0_CTLL));
	tmp = readl(mmio + HI_SATA_PHY1_CTLL);
	tmp &= ~HI_SATA_PHY_RESET;
	writel(tmp, (mmio + HI_SATA_PHY1_CTLL));

	tmp = readl(mmio + HI_SATA_PHY0_CTLH);
	tmp |= HI_SATA_BIGENDINE;
	writel(tmp, (mmio + HI_SATA_PHY0_CTLH));
	tmp = readl(mmio + HI_SATA_PHY1_CTLH);
	tmp |= HI_SATA_BIGENDINE;
	writel(tmp, (mmio + HI_SATA_PHY1_CTLH));

	/* set phy PX TX amplitude */
	for (i = 0; i < n_ports; i++) {
		tmp = HI_SATA_PX_TX_AMPLITUDE;
		writel(tmp, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL1));
	}

	/* set phy PX TX pre-emphasis */
	for (i = 0; i < n_ports; i++) {
		tmp = HI_SATA_PX_TX_PREEMPH;
		writel(tmp, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL2));
	}

	for (i = 0; i < n_ports; i++)
		writel(HI_SATA_FORCE_1_5G, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL));
	for (i = 0; i < n_ports; i++)
		writel(HI_SATA_FORCE_3G, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL));
	for (i = 0; i < n_ports; i++)
		writel(HI_SATA_FORCE_6G, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL));

	if (phy_mode == HI_SATA_PHY_MODE_1_5G)
		phy_config = HI_SATA_PHY_1_5G;
	if (phy_mode == HI_SATA_PHY_MODE_3G)
		phy_config = HI_SATA_PHY_3G;
	if (phy_mode == HI_SATA_PHY_MODE_6G)
		phy_config = HI_SATA_PHY_6G;

	for (i = 0; i < n_ports; i++)
		writel(phy_config, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL));
}
#endif
