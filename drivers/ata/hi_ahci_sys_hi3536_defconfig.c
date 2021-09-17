#ifdef CONFIG_ARCH_HI3536

#include "hi_ahci_sys_hi3536_defconfig.h"

enum {
	HI_SATA_PERI_CTRL		= IO_ADDRESS(0x12040000),
	HI_SATA_PERI_CRG43		= (HI_SATA_PERI_CTRL + 0xac),
	HI_SATA_CLK_VALUE		= 0x511,

	HI_SATA_PHY0_RST		= (1 << 2),
	HI_SATA_PHY1_RST		= (1 << 6),
	HI_SATA_PHY2_RST		= (1 << 13),
	HI_SATA_PHY3_RST		= (1 << 12),
	HI_SATA_PHY01_RST		= HI_SATA_PHY0_RST | HI_SATA_PHY1_RST,
	HI_SATA_PHY23_RST		= HI_SATA_PHY2_RST | HI_SATA_PHY3_RST,

	HI_SATA_SYS_CTRL		= IO_ADDRESS(0x1205008C),
	HI_SATA_USE_ESATA		= 16,
};

#define	HI_SATA_MISC_CTRL		IO_ADDRESS(0x12120000)
#define HI_SATA_MISC_PORT0		(HI_SATA_MISC_CTRL + 0x90)
#define HI_SATA_MISC_PORT1		(HI_SATA_MISC_CTRL + 0x94)
#define HI_SATA_MISC_COMBO		(HI_SATA_MISC_CTRL + 0x98)

static unsigned int hi_sata_use_esata(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_SYS_CTRL);
	tmp_val = (tmp_val >> HI_SATA_USE_ESATA) & 0x3;

	return (unsigned int)tmp_val;
}

static void hi_sata_poweron(void)
{
}

static void hi_sata_poweroff(void)
{
}

static void hi_sata_reset(void)
{
/**********************************************************
 * hi3536 don't need reset sata bus and clk ***************
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG45);
	tmp_val |= HI_SATA_BUS_RESET;
	tmp_val |= HI_SATA_CK0_RESET;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG45);
***********************************************************/
}

static void hi_sata_unreset(void)
{
/***********************************************************
 * hi3536 don't need unreset sata bus and clk **************
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG45);
	tmp_val &= ~(HI_SATA_BUS_RESET | HI_SATA_CK0_RESET);
	writel(tmp_val, (void *)HI_SATA_PERI_CRG45);
***********************************************************/
}

static void hi_sata_phy_reset(void)
{
	unsigned int tmp_val, nport;

	tmp_val = readl((void *)HI_SATA_PERI_CRG43);

	nport = hi_sata_use_esata();

	if (nport == 0)
		tmp_val |= HI_SATA_PHY01_RST | HI_SATA_PHY23_RST;
	if (nport == 1)
		tmp_val |= HI_SATA_PHY01_RST | HI_SATA_PHY2_RST;
	if (nport == 3)
		tmp_val |= HI_SATA_PHY01_RST;

	writel(tmp_val, (void *)HI_SATA_PERI_CRG43);
}

static void hi_sata_phy_unreset(void)
{
	unsigned int tmp_val, nport;

	tmp_val = readl((void *)HI_SATA_PERI_CRG43);

	nport = hi_sata_use_esata();

	if (nport == 0)
		tmp_val &= ~(HI_SATA_PHY01_RST | HI_SATA_PHY23_RST);
	if (nport == 1)
		tmp_val &= ~(HI_SATA_PHY01_RST | HI_SATA_PHY2_RST);
	if (nport == 3)
		tmp_val &= ~HI_SATA_PHY01_RST;

	writel(tmp_val, (void *)HI_SATA_PERI_CRG43);
}

static void hi_sata_clk_enable(void)
{
/****************************************************
 * hi3536 set CRG43 to enable sata clk **************
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG45);
	tmp_val |= HI_SATA_PORT01_CLK_EN;
	if (!(hi_sata_use_esata()))
		tmp_val |= HI_SATA_PORT2_CLK_EN;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG45);
*****************************************************/
}

static void hi_sata_clk_disable(void)
{
/****************************************************
 * hi3536 set CRG43 to disenable sata clk ***********
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG45);
	tmp_val &= ~HI_SATA_PORT01_CLK_EN;
	if (!(hi_sata_use_esata()))
		tmp_val &= ~HI_SATA_PORT2_CLK_EN;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG45);
*****************************************************/
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

	tmp_val = readl((void *)HI_SATA_PERI_CRG43);
	tmp_val |= HI_SATA_CLK_VALUE;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG43);
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
	unsigned int tmp, phy_config, phy_sg;
	int i;

	hisata_v200_set_fifo(mmio, n_ports);

	if ((n_ports < 1) || (n_ports > 4))
		pr_err("ERROR: PORT num you set is WRONG!!!\n");

	tmp = readl(mmio + HI_SATA_PHY_CTL1);
	tmp |= HI_SATA_BIGENDINE;
	writel(tmp, (mmio + HI_SATA_PHY_CTL1));
	tmp = readl(mmio + HI_SATA_PHY_CTL2);
	tmp |= HI_SATA_BIGENDINE;
	writel(tmp, (mmio + HI_SATA_PHY_CTL2));

	if (phy_mode == HI_SATA_PHY_MODE_1_5G) {
		phy_config = HI_SATA_PHY_1_5G;
		phy_sg = HI_SATA_PHY_SG_1_5G;
	}

	if (phy_mode == HI_SATA_PHY_MODE_3G) {
		phy_config = HI_SATA_PHY_3G;
		phy_sg = HI_SATA_PHY_SG_3G;
	}

	if (phy_mode == HI_SATA_PHY_MODE_6G) {
		phy_config = HI_SATA_PHY_6G;
		phy_sg = HI_SATA_PHY_SG_6G;
	}

	for (i = 0; i < n_ports; i++) {
		if (i == 0) {
			writel(0x419, (void *)HI_SATA_MISC_PORT0);
			writel(0x439, (void *)HI_SATA_MISC_PORT0);
			writel(0x419, (void *)HI_SATA_MISC_PORT0);
			writel(0x0, (void *)HI_SATA_MISC_PORT0);
		} else if (i == 1) {
			writel(0x419, (void *)HI_SATA_MISC_PORT1);
			writel(0x439, (void *)HI_SATA_MISC_PORT1);
			writel(0x419, (void *)HI_SATA_MISC_PORT1);
			writel(0x0, (void *)HI_SATA_MISC_PORT1);
		} else if (i == 2) {
			writel(0x439, (void *)HI_SATA_MISC_COMBO);
			writel(0x479, (void *)HI_SATA_MISC_COMBO);
			writel(0x439, (void *)HI_SATA_MISC_COMBO);
			writel(0x0, (void *)HI_SATA_MISC_COMBO);
		} else if (i == 3) {
			writel(0x419, (void *)HI_SATA_MISC_COMBO);
			writel(0x459, (void *)HI_SATA_MISC_COMBO);
			writel(0x419, (void *)HI_SATA_MISC_COMBO);
			writel(0x0, (void *)HI_SATA_MISC_COMBO);
		}
	}

	for (i = 0; i < n_ports; i++) {
		writel(phy_config, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL));

		writel(phy_sg, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL1));
	}
}

#endif
