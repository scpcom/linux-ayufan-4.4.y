#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/reset.h>

#include "pcie-designware.h"

#define to_ls1024a_pcie(x)	container_of(x, struct ls1024a_pcie, pp)

struct ls1024a_pcie {
	int			reset_gpio;
	struct reset_control	*axi_rst;
	struct clk		*pcie;
	struct phy		*phy;
	struct pcie_port	pp;
	struct regmap		*app;
	int			app_profile;
	struct gpio_desc	*gpiod_reset;
};


/* The following register definitions are as per "DWC_regs_rev04.doc" document */

struct pcie_app_reg {
	u32	cfg0;
	u32	cfg1;
	u32	cfg2;
	u32	cfg3;
	u32	cfg4;
	u32	cfg5;
	u32	cfg6;
	u32	sts0;
	u32	sts1;
	u32	sts2;
	u32	sts3;
	u32	pwr_cfg_bdgt_data;
	u32	pwr_cfg_bdgt_fn;
	u32	radm_sts;
	u32	pwr_sts_bdgt;
	u32	intr_sts;
	u32	intr_en;
	u32	intr_msi_sts;
	u32	intr_msi_en;
};

#define MAX_PCIE_PORTS  2
/* DWC PCIEe configuration register offsets on APB */
struct pcie_app_reg app_regs[MAX_PCIE_PORTS] = {
	/* PCIe0 */
	{
		0x00000000, /* cfg0 */
		0x00000004,
		0x00000008,
		0x0000000C,
		0x00000010,
		0x00000014,
		0x00000018, /* cfg6 */
		0x00000040, /* sts0 */
		0x00000044,
		0x00000048,
		0x00000058, /* sts3 */
		0x00000080,
		0x00000084,
		0x000000C0, /* radm_sts */
		0x000000C4,
		0x00000100, /* intr_sts */
		0x00000104,
		0x00000108,
		0x0000010C
	},
	/* PCIe1 */
	{
		0x00000020,
		0x00000024,
		0x00000028,
		0x0000002C,
		0x00000030,
		0x00000034,
		0x00000038,
		0x0000004C,
		0x00000050,
		0x00000054,
		0x0000005C,
		0x00000088,
		0x0000008C,
		0x000000C8,
		0x000000CC,
		0x00000110,
		0x00000114,
		0x00000118,
		0x0000011C
	}
};

/* INTR_STS and INTR_EN Register definitions */
#define 	INTR_CTRL_INTA_ASSERT	0x0001
#define 	INTR_CTRL_INTA_DEASSERT	0x0002
#define 	INTR_CTRL_INTB_ASSERT	0x0004
#define 	INTR_CTRL_INTB_DEASSERT	0x0008
#define 	INTR_CTRL_INTC_ASSERT	0x0010
#define 	INTR_CTRL_INTC_DEASSERT	0x0020
#define 	INTR_CTRL_INTD_ASSERT	0x0040
#define 	INTR_CTRL_INTD_DEASSERT	0x0080
#define 	INTR_CTRL_AER		0x0100
#define 	INTR_CTRL_PME		0x0200
#define 	INTR_CTRL_HP		0x0400
#define 	INTR_CTRL_LINK_AUTO_BW	0x0800
#define 	INTR_CTRL_MSI		0x1000
static irqreturn_t ls1024a_pcie_msi_handler(int irq, void *arg)
{
	struct pcie_port *pp = arg;
	struct ls1024a_pcie *ls1024a_pcie = to_ls1024a_pcie(pp);
	irqreturn_t ret = IRQ_NONE;
	u32 status = 0;

	regmap_read(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].intr_sts, &status);
	if (status & INTR_CTRL_MSI) {
		regmap_write(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].intr_sts, INTR_CTRL_MSI);
		status &= ~INTR_CTRL_MSI;
		ret = dw_handle_msi_irq(pp);
	}
	regmap_write(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].intr_sts, status);
	if (status)
		return IRQ_NONE;
	else
		return ret;

}

static int ls1024a_pcie_wait_for_link(struct pcie_port *pp)
{
	int count = 200;

	while (!dw_pcie_link_up(pp)) {
		usleep_range(100, 1000);
		if (--count)
			continue;

		dev_err(pp->dev, "phy link never came up\n");
		return -EINVAL;
	}

	return 0;
}

/* Port Logic Registers */
#define PCIE_ALRT_REG              0x700
#define PCIE_AFL0L1_REG            0x70C
#define PCIE_SYMNUM_REG            0x718
#define PCIE_G2CTRL_REG            0x80C

#define PCIE_CAP_BASE           0x70
#define PCIE_LCAP_REG           (PCIE_CAP_BASE + 0x0C)
#define PCIE_LCNT2_REG          (PCIE_CAP_BASE + 0x30)

static void ls1024a_pcie_host_init(struct pcie_port *pp)
{
	struct ls1024a_pcie *ls1024a_pcie = to_ls1024a_pcie(pp);
	int ret;
	u32 val;

	ret = reset_control_deassert(ls1024a_pcie->axi_rst);
	ret = phy_init(ls1024a_pcie->phy);
	/* TODO: Do we really need this delay */
	mdelay(1); //After CMU locks wait for sometime

	/* TODO: PCIe_SATA_RST_CNTRL */
#define 	CFG5_APP_INIT_RST	0x01
#define 	CFG5_LTSSM_ENABLE	0x02
	//Hold the LTSSM in detect state
	regmap_update_bits(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].cfg5, CFG5_LTSSM_ENABLE, 0);

	//FIXME : Bit:27 definition is not clear from document
	//	  This register setting is copied from simulation code.
	regmap_update_bits(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].cfg0, 0x08007FF0, 0x08007FF0);

	val = readl(pp->dbi_base + PCIE_AFL0L1_REG);
	val &= ~(0x00FFFF00);
	val |= 0x00F1F100;
	writel(val, pp->dbi_base + PCIE_AFL0L1_REG);

	if(0) // if (pcie_gen1_only)
	{
		writel(0x1, pp->dbi_base + PCIE_LCNT2_REG);
		writel(0x1, pp->dbi_base + PCIE_LCAP_REG);
	}
	else
	{
		val = readl(pp->dbi_base + PCIE_G2CTRL_REG);
		val &= ~(0xFF);
		val |= 0xF1;
		writel(val, pp->dbi_base + PCIE_G2CTRL_REG);

		// instruct pcie to switch to gen2 after init
		val = readl(pp->dbi_base + PCIE_G2CTRL_REG);
		val |= (1 << 17);
		writel(val, pp->dbi_base + PCIE_G2CTRL_REG);
	}

	dw_pcie_setup_rc(pp);

	//Enable LTSSM to start link initialization
	regmap_update_bits(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].cfg5,
			(CFG5_APP_INIT_RST | CFG5_LTSSM_ENABLE),
			(CFG5_APP_INIT_RST | CFG5_LTSSM_ENABLE));

	ls1024a_pcie_wait_for_link(&ls1024a_pcie->pp);

	if (IS_ENABLED(CONFIG_PCI_MSI))
		dw_pcie_msi_init(pp);

}

#define 	STS0_RDLH_LINK_UP	0x10000
static int ls1024a_pcie_link_up(struct pcie_port *pp)
{
	struct ls1024a_pcie *ls1024a_pcie = to_ls1024a_pcie(pp);
	u32 rc = 0;
	int count = 5;

	while (1) {
		regmap_read(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].sts0, &rc);
		if (rc & STS0_RDLH_LINK_UP)
			return 1;
		if (!count--)
			break;
		/*
		 * Wait a little bit, then re-check if the link finished
		 * the training.
		 */
		usleep_range(1000, 2000);
	}
	return 0;
}

static struct pcie_host_ops ls1024a_pcie_host_ops = {
	.link_up = ls1024a_pcie_link_up,
	.host_init = ls1024a_pcie_host_init,
};

static int __init ls1024a_add_pcie_port(struct pcie_port *pp,
			struct platform_device *pdev)
{
	int ret;

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		pp->msi_irq = platform_get_irq_byname(pdev, "msi");
		if (pp->msi_irq <= 0) {
			dev_err(&pdev->dev, "failed to get MSI irq\n");
			return -ENODEV;
		}

		ret = devm_request_irq(&pdev->dev, pp->msi_irq,
				       ls1024a_pcie_msi_handler,
				       IRQF_SHARED, "ls1024a-pcie-msi", pp);
		if (ret) {
			dev_err(&pdev->dev, "failed to request MSI irq\n");
			return -ENODEV;
		}
	}

	pp->root_bus_nr = -1;
	pp->ops = &ls1024a_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

static int __init ls1024a_pcie_probe(struct platform_device *pdev)
{
	struct ls1024a_pcie *ls1024a_pcie;
	struct pcie_port *pp;
	struct device_node *np = pdev->dev.of_node;
	struct resource *dbi_base;
	int ret;

	ls1024a_pcie = devm_kzalloc(&pdev->dev, sizeof(*ls1024a_pcie), GFP_KERNEL);
	if (!ls1024a_pcie)
		return -ENOMEM;

	pp = &ls1024a_pcie->pp;
	pp->dev = &pdev->dev;

	/* Added for PCI abort handling */
#if 0
	hook_fault_code(16 + 6, ls1024aq_pcie_abort_handler, SIGBUS, 0,
		"imprecise external abort");
#endif

	dbi_base = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	pp->dbi_base = devm_ioremap_resource(&pdev->dev, dbi_base);
	if (IS_ERR(pp->dbi_base))
		return PTR_ERR(pp->dbi_base);
#if 0
	app = platform_get_resource_byname(pdev, IORESOURCE_MEM, "app");
	ls1024a_pcie->app = devm_ioremap_resource(&pdev->dev, app);
	if (IS_ERR(ls1024a_pcie->app))
		return PTR_ERR(ls1024a_pcie->app);
#endif

	ls1024a_pcie->app = syscon_regmap_lookup_by_phandle(np, "app-syscon");
	if (IS_ERR(ls1024a_pcie->app)) {
		dev_err(&pdev->dev,
			"Error retrieving pci_sata_usb_ctrl syscon\n");
		return PTR_ERR(ls1024a_pcie->app);
	}

	ls1024a_pcie->app_profile = 0;
	of_property_read_u32(np, "app-profile", &ls1024a_pcie->app_profile);
	/* Fetch clocks */
	ls1024a_pcie->pcie = devm_clk_get(&pdev->dev, "pcie");
	if (IS_ERR(ls1024a_pcie->pcie)) {
		dev_err(&pdev->dev,
			"pcie clock source missing or invalid\n");
		return PTR_ERR(ls1024a_pcie->pcie);
	}
	ls1024a_pcie->phy = devm_of_phy_get(pp->dev, np, NULL);
	if (IS_ERR(ls1024a_pcie->phy)) {
		ret = PTR_ERR(ls1024a_pcie->phy);
		if (ret == -EPROBE_DEFER) {
			return ret;
		} else if (ret != -ENOSYS && ret != -ENODEV) {
			dev_err(pp->dev,
				"Error retrieving PCIe phy: %d\n", ret);
			return ret;
		}
	}

	ls1024a_pcie->axi_rst = devm_reset_control_get(pp->dev, "axi");
	if (IS_ERR(ls1024a_pcie->axi_rst))
		return PTR_ERR(ls1024a_pcie->axi_rst);

	ls1024a_pcie->gpiod_reset = devm_gpiod_get_optional(pp->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ls1024a_pcie->gpiod_reset)) {
		ls1024a_pcie->gpiod_reset = 0;
	}
	if (ls1024a_pcie->gpiod_reset) {
		pr_err("Pulsing GPIO reset line\n");
		gpiod_set_value(ls1024a_pcie->gpiod_reset, 1);
		msleep(1);
		gpiod_set_value(ls1024a_pcie->gpiod_reset, 0);
		msleep(1);
	}
	/* TODO */
#define 	CFG0_DEV_TYPE_RC 	0x04
	regmap_update_bits(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].cfg0, 0xf, CFG0_DEV_TYPE_RC);

	ret = ls1024a_add_pcie_port(pp, pdev);
	if (ret < 0)
		return ret;
	/* TODO */
	regmap_update_bits(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].intr_en, INTR_CTRL_INTA_ASSERT, INTR_CTRL_INTA_ASSERT);
	regmap_update_bits(ls1024a_pcie->app, app_regs[ls1024a_pcie->app_profile].intr_en, INTR_CTRL_MSI, INTR_CTRL_MSI);

	platform_set_drvdata(pdev, ls1024a_pcie);
	return 0;
}

static const struct of_device_id ls1024a_pcie_of_match[] = {
	{ .compatible = "fsl,ls1024a-pcie", },
	{},
};
MODULE_DEVICE_TABLE(of, ls1024a_pcie_of_match);

static struct platform_driver ls1024a_pcie_driver = {
	.driver = {
		.name	= "ls1024a-pcie",
		.of_match_table = ls1024a_pcie_of_match,
	},
};

static int __init ls1024a_pcie_init(void)
{
	return platform_driver_probe(&ls1024a_pcie_driver, ls1024a_pcie_probe);
}
module_init(ls1024a_pcie_init);

MODULE_AUTHOR("Daniel Mentz <danielmentz@google.com>");
MODULE_DESCRIPTION("Freescale LS1024A PCIe host controller driver");
MODULE_LICENSE("GPL v2");
