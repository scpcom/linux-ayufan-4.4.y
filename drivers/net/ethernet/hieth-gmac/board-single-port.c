/* bit define for CRG_GMAC register */
#define BIT_GSF_SOFT_RESET		BIT(0)
#define BIT_GSF_CLK_EN			BIT(1)
#define BIT_GSF_MAC_IF_SOFT_RESET	BIT(2)
#define BIT_GSF_MAC_IF_CLK_EN		BIT(3)
#define BIT_RMII_CLK_SELECT		BIT(4)
#define BIT_EXT_PHY_RESET		BIT(5)
#define BIT_EXT_PHY_CLK_SELECT		BIT(6)

void __iomem *soc_fwdctl_iobase(void)
{
	/* there is no fwd module in 3516a */
	return NULL;
}

void higmac_hw_mac_core_reset(struct higmac_netdev_local *ld)
{
	struct higmac_adapter *adapter = ld->adapter;
	void __iomem *p_mac_crg;
	unsigned int v = 0;
	unsigned long flags;

	p_mac_crg = (void __iomem *)HIGMAC_CRG_IOBASE;

	spin_lock_irqsave(&adapter->lock, flags);

	v = readl(p_mac_crg);
	v |= (BIT_GSF_CLK_EN | BIT_GSF_MAC_IF_CLK_EN); /* enable clk */
	writel(v, p_mac_crg);

	/* phy clk select */
	v = readl(p_mac_crg);
	v |= BIT_EXT_PHY_CLK_SELECT;
	writel(v, p_mac_crg);

#ifdef CONFIG_HIGMAC_RMII_CLK_USE_EXTERNAL_PAD
	if (higmac_board_info[ld->index].phy_intf == PHY_INTERFACE_MODE_RMII) {
		v = readl(p_mac_crg);
		v |= BIT_RMII_CLK_SELECT; /* rmii select pad clk */
		writel(v, p_mac_crg);
	}
#endif

	/* set reset bit */
	v = readl(p_mac_crg);
	v |= BIT_GSF_SOFT_RESET;
	writel(v, p_mac_crg);
	spin_unlock_irqrestore(&adapter->lock, flags);

	udelay(50);

	spin_lock_irqsave(&adapter->lock, flags);
	/* clear reset bit */
	v = readl(p_mac_crg);
	v &= ~BIT_GSF_SOFT_RESET;
	writel(v, p_mac_crg);

	spin_unlock_irqrestore(&adapter->lock, flags);
}

void higmac_set_macif(struct higmac_netdev_local *ld, int mode, int speed)
{
	struct higmac_adapter *adapter = ld->adapter;
	void __iomem *p_mac_crg;
	unsigned long flags;
	unsigned int v;

	p_mac_crg = (void __iomem *)HIGMAC_CRG_IOBASE;

	/* enable change: port_mode */
	higmac_writel_bits(ld, 1, MODE_CHANGE_EN, BIT_MODE_CHANGE_EN);
	if (speed == 2)/* FIXME */
		speed = 5;/* 1000M */
	higmac_writel_bits(ld, speed, PORT_MODE, BITS_PORT_MODE);
	/* disable change: port_mode */
	higmac_writel_bits(ld, 0, MODE_CHANGE_EN, BIT_MODE_CHANGE_EN);

	spin_lock_irqsave(&adapter->lock, flags);
	/* soft reset mac_if */
	v = readl(p_mac_crg);
	v |= BIT_GSF_MAC_IF_SOFT_RESET;
	writel(v, p_mac_crg);

	/* config mac_if */
	writel(mode, (void __iomem *)HIGMAC_MAC_IF_IOBASE);

	/* undo reset */
	v = readl(p_mac_crg);
	v &= ~BIT_GSF_MAC_IF_SOFT_RESET;
	writel(v, p_mac_crg);
	spin_unlock_irqrestore(&adapter->lock, flags);
}

void higmac_hw_internal_fephy_reset(struct higmac_adapter *adapter)
{
}

void higmac_hw_external_phy_reset(void)
{
	void __iomem *p_mac_crg;
	unsigned int v;

	p_mac_crg = (void __iomem *)HIGMAC_CRG_IOBASE;

	/* use CRG register to reset external phy */
	v = readl(p_mac_crg);
	v |= BIT_EXT_PHY_RESET; /* reset */
	writel(v, p_mac_crg);

	mdelay(50); /* wait for phy reset time */

	v = readl(p_mac_crg);
	v &= ~BIT_EXT_PHY_RESET; /*undo reset */
	writel(v, p_mac_crg);

	mdelay(60); /* wait for future MDIO operation */
}

void higmac_hw_all_clk_disable(void)
{
}
