#ifdef CONFIG_ARCH_HI3536

#define SDIO_REG_BASE_CRG               IO_ADDRESS(0x12040000)

/* SDIO0 REG */
#define PERI_CRG49			0xC4

#define SDIO0_CLK_SEL_MASK		(0x3 << 2)

#define SDIO0_CLK_PCTRL			(0x1 << 4)
#define SDIO0_CLK_SEL_50M		(0x1 << 2)
#define SDIO0_CLK_SEL_24M		(0x0 << 2)

#define SDIO0_SRST_REQ			(0x1 << 0)
#define SDIO0_CKEN			(0x1 << 1)
#define SDIO0_RESET			(0x1 << 0)

/* SDIO1 REG */
#define PERI_CRG132			0x210

#define SDIO1_CLK_SEL_MASK		(0x3 << 4)

#define SDIO1_CLK_SEL_75M		(0x3 << 4)
#define SDIO1_CLK_SEL_100M		(0x1 << 4)
#define SDIO1_CLK_SEL_50M		(0x0 << 4)
#define SDIO1_CLK_SEL_25M		(0x2 << 4)

#define SDIO1_SRST_REQ			(0x1 << 0)
#define SDIO1_CKEN			(0x1 << 1)
#define SDIO1_RESET			(0x1 << 0)

/*UHS REG EXT*/
#define MCI_UHS_REG_EXT			0x108

#define MMC_DRV_PS_SEL_MASK		(0x7 << 23)
#define MMC_SAP_PS_SEL_MASK		(0x7 << 16)

#define MMC_DRV_PS_SEL_0		(0x0 << 23)
#define MMC_DRV_PS_SEL_45		(0x1 << 23)
#define MMC_DRV_PS_SEL_90		(0x2 << 23)
#define MMC_DRV_PS_SEL_135		(0x3 << 23)
#define MMC_DRV_PS_SEL_180		(0x4 << 23)
#define MMC_DRV_PS_SEL_225		(0x5 << 23)
#define MMC_DRV_PS_SEL_270		(0x6 << 23)
#define MMC_DRV_PS_SEL_315		(0x7 << 23)

#define MMC_SAP_PS_SEL_0		(0x0 << 16)
#define MMC_SAP_PS_SEL_45		(0x1 << 16)
#define MMC_SAP_PS_SEL_90		(0x2 << 16)
#define MMC_SAP_PS_SEL_135		(0x3 << 16)
#define MMC_SAP_PS_SEL_180		(0x4 << 16)
#define MMC_SAP_PS_SEL_225		(0x5 << 16)
#define MMC_SAP_PS_SEL_270		(0x6 << 16)
#define MMC_SAP_PS_SEL_315		(0x7 << 16)

static void hi_mci_sys_ctrl_init(struct himci_host *host,
				 resource_size_t host_crg_addr)
{
	unsigned int tmp_reg = 0;

	if ((SDIO_REG_BASE_CRG + PERI_CRG49) == (unsigned int)host_crg_addr) {
#ifdef CONFIG_HIMCIV200_SDIO0
		/* enable SDIO clock */
		tmp_reg = himci_readl(host_crg_addr);

		tmp_reg &= ~SDIO0_CLK_SEL_MASK;
		if (CONFIG_HIMCIV200_SDIO0_CLK == 24000000)
			tmp_reg |= SDIO0_CLK_SEL_24M;
		else
			tmp_reg |= SDIO0_CLK_SEL_50M; /* SDIO0 default 50Hz */
		himci_writel(tmp_reg, host_crg_addr);

		/* SDIO soft reset */
		tmp_reg = himci_readl(host_crg_addr);
		tmp_reg |= SDIO0_SRST_REQ;
		himci_writel(tmp_reg, host_crg_addr);
		udelay(1000);
		tmp_reg &= ~SDIO0_SRST_REQ;
		tmp_reg |= SDIO0_CKEN;
		himci_writel(tmp_reg, host_crg_addr);
#endif
		return;
	}

	if ((SDIO_REG_BASE_CRG + PERI_CRG132) == (unsigned int)host_crg_addr) {
#ifdef CONFIG_HIMCIV200_SDIO1
		/* SDIO clock phase */
		tmp_reg = himci_readl(host_crg_addr);
		tmp_reg &= ~SDIO1_CLK_SEL_MASK;

		if (CONFIG_HIMCIV200_SDIO1_CLK == 25000000)
			tmp_reg |= SDIO1_CLK_SEL_25M;
		else
			tmp_reg |= SDIO1_CLK_SEL_50M; /* SDIO1 default 50Hz */
		himci_writel(tmp_reg, host_crg_addr);

		/* SDIO soft reset */
		tmp_reg |= SDIO1_SRST_REQ;
		himci_writel(tmp_reg, host_crg_addr);
		udelay(1000);
		tmp_reg &= ~SDIO1_SRST_REQ;
		tmp_reg |= SDIO1_CKEN;
		himci_writel(tmp_reg, host_crg_addr);
#endif
		return;
	}

	return;
}

static void hi_mci_sys_ctrl_suspend(struct himci_host *host,
				    resource_size_t host_crg_addr)
{
	unsigned int tmp_reg = 0;

	if ((SDIO_REG_BASE_CRG + PERI_CRG49) == (unsigned int)host_crg_addr) {

		/* SDIO soft reset */
		tmp_reg = himci_readl(host_crg_addr);
		tmp_reg |= SDIO0_SRST_REQ;
		himci_writel(tmp_reg, host_crg_addr);
		udelay(1000);

		/* disable SDIO clock */
		tmp_reg &= ~SDIO0_CKEN;
		himci_writel(tmp_reg, host_crg_addr);
		return;
	}

	if ((SDIO_REG_BASE_CRG + PERI_CRG132) == (unsigned int)host_crg_addr) {

		/* SDIO soft reset */
		tmp_reg |= SDIO1_SRST_REQ;
		himci_writel(tmp_reg, host_crg_addr);
		udelay(1000);
		tmp_reg &= ~SDIO0_CKEN;
		himci_writel(tmp_reg, host_crg_addr);
		return;
	}

	return;
}
static void himci_ctr_reset(struct himci_host *host)
{
	unsigned int reg_value;

	if (0 == host->devid) {
		reg_value = himci_readl(SDIO_REG_BASE_CRG + PERI_CRG49);
		reg_value |= SDIO0_RESET;
		himci_writel(reg_value, SDIO_REG_BASE_CRG + PERI_CRG49);
	} else if (1 == host->devid) {
		reg_value = himci_readl(SDIO_REG_BASE_CRG + PERI_CRG132);
		reg_value |= SDIO1_RESET;
		himci_writel(reg_value, SDIO_REG_BASE_CRG + PERI_CRG132);
	} else {
		himci_error("himci host id error!");
		return;
	}
}

static void himci_ctr_undo_reset(struct himci_host *host)
{
	unsigned int reg_value;

	if (0 == host->devid) {
		reg_value = himci_readl(SDIO_REG_BASE_CRG + PERI_CRG49);
		reg_value &= ~(SDIO0_RESET);
		himci_writel(reg_value, SDIO_REG_BASE_CRG + PERI_CRG49);
	} else if (1 == host->devid) {
		reg_value = himci_readl(SDIO_REG_BASE_CRG + PERI_CRG132);
		reg_value &= ~(SDIO1_RESET);
		himci_writel(reg_value, SDIO_REG_BASE_CRG + PERI_CRG132);
	} else {
		himci_error("himci host id error!");
		return;
	}
}

#endif /* CONFIG_ARCH_HI3536 */
