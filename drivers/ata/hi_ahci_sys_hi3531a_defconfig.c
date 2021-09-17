#ifdef CONFIG_ARCH_HI3531A

#include "hi_ahci_sys_hi3531a_defconfig.h"

enum {
	HI_SATA_PERI_CTRL		= IO_ADDRESS(0x12040000),
	HI_SATA_PERI_CRG72		= (HI_SATA_PERI_CTRL + 0x120),
	HI_SATA_CLK_VALUE		= 0xa3a3,

	HI_SATA_PHY0A_RST		= (1 << 2),
	HI_SATA_PHY0B_RST		= (1 << 3),
	HI_SATA_PHY1A_RST		= (1 << 10),
	HI_SATA_PHY1B_RST		= (1 << 11),
	HI_SATA_PHY0_RST		= HI_SATA_PHY0A_RST | HI_SATA_PHY0B_RST,
	HI_SATA_PHY1_RST		= HI_SATA_PHY1A_RST | HI_SATA_PHY1B_RST,

	HI_SATA_PHY0A_RST_MASK		= (1 << 7),
	HI_SATA_PHY0B_RST_MASK		= (1 << 6),
	HI_SATA_PHY1A_RST_MASK		= (1 << 5),
	HI_SATA_PHY1B_RST_MASK		= (1 << 4),
	HI_SATA_PHY0_RST_MASK		= HI_SATA_PHY0A_RST_MASK
					| HI_SATA_PHY0B_RST_MASK,

	HI_SATA_PHY1_RST_MASK		= HI_SATA_PHY1A_RST_MASK
					| HI_SATA_PHY1B_RST_MASK,

	HI_SATA_SYS_CTRL		= IO_ADDRESS(0x1205008C),
	HI_SATA_USE_ESATA		= 12,
};

#define	HI_SATA_MISC_CTRL		IO_ADDRESS(0x12120000)
#define HI_SATA_MISC_COMB_PHY0		(HI_SATA_MISC_CTRL + 0x134)
#define HI_SATA_MISC_COMB_PHY1		(HI_SATA_MISC_CTRL + 0x138)

unsigned int mplx_port0;
static unsigned int sata_port_nr;

static unsigned int hi_sata_use_esata(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_SYS_CTRL);
	tmp_val = (tmp_val >> HI_SATA_USE_ESATA) & 0xf;

	return (unsigned int)tmp_val;
}

static unsigned int hi_sata_port_nr(void)
{
	unsigned int mode, port_nr;

	mode =  hi_sata_use_esata();
	switch (mode) {
	case 0x0:
		port_nr = 4;
		break;

	case 0x1:
	case 0x8:
		port_nr = 3;
		break;

	case 0x2:
	case 0x3:
	case 0x9:
		port_nr = 2;
		break;

	case 0xa:
	case 0xb:
		port_nr = 1;
		break;

	default:
		port_nr = 0;
		break;
	}

	mplx_port0 = (mode & 0x8) ? 1 : 0;
	sata_port_nr = port_nr;

	return port_nr;
}

static void hi_sata_poweron(void)
{
}

static void hi_sata_poweroff(void)
{
}

static void hi_sata_reset(void)
{
}

static void hi_sata_unreset(void)
{
}

static void hi_sata_phy_reset(void)
{
	unsigned int tmp_val, nport;

	tmp_val = readl((void *)HI_SATA_PERI_CRG72);

	nport = sata_port_nr;

	if (nport == 4)
		tmp_val |= HI_SATA_PHY0_RST | HI_SATA_PHY1_RST;
	if (nport == 3) {
		if (mplx_port0)
			tmp_val |= HI_SATA_PHY1A_RST | HI_SATA_PHY0_RST;
		else
			tmp_val |= HI_SATA_PHY1_RST | HI_SATA_PHY0A_RST;
	}
	if (nport == 2) {
		if (mplx_port0)
			tmp_val |= HI_SATA_PHY1A_RST | HI_SATA_PHY0B_RST;
		else
			tmp_val |= HI_SATA_PHY1_RST;
	}
	if (nport == 1)
		tmp_val |= HI_SATA_PHY1A_RST;

	writel(tmp_val, (void *)HI_SATA_PERI_CRG72);
}

static void hi_sata_phy_unreset(void)
{
	unsigned int tmp_val, nport;

	tmp_val = readl((void *)HI_SATA_PERI_CRG72);

	nport = sata_port_nr;

	if (nport == 4)
		tmp_val &= ~(HI_SATA_PHY0_RST | HI_SATA_PHY1_RST);
	if (nport == 3) {
		if (mplx_port0)
			tmp_val &= ~(HI_SATA_PHY1A_RST | HI_SATA_PHY0_RST);
		else
			tmp_val &= ~(HI_SATA_PHY1_RST | HI_SATA_PHY0A_RST);
	}
	if (nport == 2) {
		if (mplx_port0)
			tmp_val &= ~(HI_SATA_PHY1A_RST | HI_SATA_PHY0B_RST);
		else
			tmp_val &= ~HI_SATA_PHY1_RST;
	}
	if (nport == 1)
		tmp_val &= ~HI_SATA_PHY1A_RST;

	writel(tmp_val, (void *)HI_SATA_PERI_CRG72);
}

static void hi_sata_clk_enable(void)
{
}
static void hi_sata_clk_disable(void)
{
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

	tmp_val = readl((void *)HI_SATA_PERI_CRG72);
	tmp_val |= HI_SATA_CLK_VALUE;
	writel(tmp_val, (void *)HI_SATA_PERI_CRG72);
}

void hisata_v200_set_fifo(void *mmio, int n_ports)
{
	int i, port_no;

	for (i = 0; i < n_ports; i++) {
		port_no = i;
		if (mplx_port0)
			port_no++;

		writel(HI_SATA_FIFOTH_VALUE, (mmio + 0x100 + port_no*0x80
					+ HI_SATA_PORT_FIFOTH));
	}
}

void hisata_phy_init(void *mmio, int phy_mode, int n_ports)
{
	unsigned int tmp, phy_config, phy_sg;
	int i, port_no;

	hisata_v200_set_fifo(mmio, n_ports);

	tmp = readl(mmio + HI_SATA_PHY_CTL1);
	tmp |= HI_SATA_BIGENDINE;
	writel(tmp, (mmio + HI_SATA_PHY_CTL1));
	tmp = readl(mmio + HI_SATA_PHY_CTL2);
	tmp |= HI_SATA_BIGENDINE;
	writel(tmp, (mmio + HI_SATA_PHY_CTL2));

	tmp = readl(mmio + HI_SATA_RST_PHY_MASK);
	tmp &= 0xffffff0f;
	if (n_ports == 1)
		tmp |= HI_SATA_PHY0_RST_MASK | HI_SATA_PHY1B_RST_MASK;
	if (n_ports == 2) {
		if (mplx_port0)
			tmp |= HI_SATA_PHY1B_RST_MASK | HI_SATA_PHY0A_RST_MASK;
		else
			tmp |= HI_SATA_PHY0_RST_MASK;	/* mode:2,3 */
	}
	if (n_ports == 3) {
		if (mplx_port0)
			tmp |= HI_SATA_PHY1B_RST_MASK;	/* mode:8 */
		else
			tmp |= HI_SATA_PHY0A_RST_MASK;	/* mode:1 */
	}
	writel(tmp, (mmio + HI_SATA_RST_PHY_MASK));

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
		port_no = i;
		if (mplx_port0)
			port_no++;

		if (port_no == 0) {
			writel(0x439, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x479, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x439, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

			writel(0x822, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x862, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x822, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

			writel(0x421, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x461, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x421, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);
		} else if (port_no == 1) {
			writel(0x419, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x459, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x419, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

			writel(0x802, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x842, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x802, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

			writel(0x401, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x441, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x401, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);
		} else if (port_no == 2) {
			writel(0x439, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x479, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x439, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

			writel(0x822, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x862, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x822, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

			writel(0x421, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x461, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x421, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);
		} else if (port_no == 3) {
			writel(0x419, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x459, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x419, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

			writel(0x802, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x842, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x802, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

			writel(0x401, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x441, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x401, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);
		}
	}

	for (i = 0; i < n_ports; i++) {
		port_no = i;
		if (mplx_port0)
			port_no++;

		writel(phy_config, (mmio + 0x100 + port_no*0x80
					+ HI_SATA_PORT_PHYCTL));

		writel(phy_sg, (mmio + 0x100 + port_no*0x80
					+ HI_SATA_PORT_PHYCTL1));
	}
}

#endif
