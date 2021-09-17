/* suppose IO_ADDRESS cover all these address range */
#define HIGMAC_FWD_IOBASE	(IO_ADDRESS(CONFIG_HIGMAC_IOBASE + 0x2000))
#define HIGMAC_MAC0_IF_CTRL	((void __iomem *)(IO_ADDRESS( \
					CONFIG_HIGMAC_IOBASE + 0x300C)))
#define HIGMAC_MAC1_IF_CTRL	((void __iomem *)(IO_ADDRESS( \
					CONFIG_HIGMAC_IOBASE + 0x3010)))

void __iomem *soc_fwdctl_iobase(void)
{
	return (void __iomem *)HIGMAC_FWD_IOBASE;
}

void higmac_hw_mac_core_reset(struct higmac_netdev_local *ld)
{
	struct higmac_adapter *adapter = ld->adapter;
	void __iomem *p_mac_crg;
	unsigned int v = 0, index;
	unsigned long flags;

	p_mac_crg = (void __iomem *)HIGMAC_CRG_IOBASE;

	spin_lock_irqsave(&adapter->lock, flags);

	/* TODO: enable clk here. fpga use fixed clk */
	v = 0x7f; /* enable clk, select DPLL */
	writel(v, p_mac_crg);

	/* set reset bit */
	index = ld->index + 8;/* 8--reset bit offset */
	v = readl(p_mac_crg);
	v |= 0x5 << index;
	writel(v, p_mac_crg);
	spin_unlock_irqrestore(&adapter->lock, flags);

	udelay(50);

	spin_lock_irqsave(&adapter->lock, flags);
	/* clear reset bit */
	v = readl(p_mac_crg);
	v &= ~(0x5 << index);
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
	v |= 1 << (ld->index + 10);/* bit10 for macif0 */
	writel(v, p_mac_crg);

	/* config mac_if */
	if (ld->index)/* eth1 */
		writel((u32)mode, HIGMAC_MAC1_IF_CTRL);
	else
		writel((u32)mode, HIGMAC_MAC0_IF_CTRL);

	/* undo reset */
	v = readl(p_mac_crg);
	v &= ~(1 << (ld->index + 10));
	writel(v, p_mac_crg);
	spin_unlock_irqrestore(&adapter->lock, flags);
}

void higmac_hw_internal_fephy_reset(struct higmac_adapter *adapter)
{
}

void higmac_hw_external_phy_reset(void)
{
}

void higmac_hw_all_clk_disable(void)
{
}

void higmac_hw_all_clk_enable(void)
{
}
