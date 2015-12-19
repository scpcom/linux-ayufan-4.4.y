/*
 * PHY driver for NXP QorIQ LS1024A USB 2.0 OTG PHY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/io.h>

#define USB0_PHY_CTRL_REG0	0x000
#define DWC_CFG_REGF		0x03C

struct ls1024a_usb2_otg_phy {
	struct phy *phy;
	struct clk *clk;
	void __iomem *reg;
	struct regmap *pci_sata_usb_ctrl_reg;
	struct reset_control *rstc_phy;
	struct reset_control *rstc_utmi;
	struct reset_control *rstc_axi;
};

static int ls1024a_usb2_otg_phy_init(struct phy *phy)
{
	struct ls1024a_usb2_otg_phy *lu2p = phy_get_drvdata(phy);
	int ret;

	ret = clk_prepare_enable(lu2p->clk);
	if (ret)
		return ret;

	/*
	 * DWC_CFG_REGF
	 *
	 * Bit[8]: USB0_iddig select line either from PHY utmiotg_iddig or from
	 * the register bit:
	 *   0: Selects iddig from PHY.
	 *   1: Selects from the register bit (Bit[9])
	 * Bit[9]:Sets whether the connected plug is a mini-A or mini-B plug RW
	 * if usb0_id_sel register is programmed to be 1:
	 *   0: sets Mini-A
	 *   1: sets Mini-B
	 *
	 * Bit[12] and Bit[13] are the equivalents of Bit[8] and Bit[9] for the
	 * second USB port. (The LS1024A has only one USB2.0 port anyway)
	 *
	 * We force both ports to Mini-A because we always operate in host
	 * mode. Also, we are not using Mini-USB connectors in the first place.
	 * All our boards have Type A receptacles.
	 */
	regmap_update_bits(lu2p->pci_sata_usb_ctrl_reg, DWC_CFG_REGF, 0x0000FF00, 0x00001100);

	reset_control_assert(lu2p->rstc_utmi);
	reset_control_assert(lu2p->rstc_phy);
	reset_control_assert(lu2p->rstc_axi);

	reset_control_deassert(lu2p->rstc_phy);
	reset_control_deassert(lu2p->rstc_utmi);
	reset_control_deassert(lu2p->rstc_axi);

	return 0;
}

static int ls1024a_usb2_otg_phy_exit(struct phy *phy)
{
	struct ls1024a_usb2_otg_phy *lu2p = phy_get_drvdata(phy);

	reset_control_assert(lu2p->rstc_utmi);
	reset_control_assert(lu2p->rstc_phy);
	reset_control_assert(lu2p->rstc_axi);

	clk_disable_unprepare(lu2p->clk);

	return 0;
}

static const struct phy_ops ls1024a_usb2_otg_phy_ops = {
	.init		= ls1024a_usb2_otg_phy_init,
	.exit		= ls1024a_usb2_otg_phy_exit,
	.owner		= THIS_MODULE,
};

static int ls1024a_usb2_otg_phy_probe(struct platform_device *pdev)
{
	struct phy_provider *phy_provider;
	struct ls1024a_usb2_otg_phy *lu2p;
	struct resource *res;

	lu2p = devm_kzalloc(&pdev->dev, sizeof(*lu2p), GFP_KERNEL);
	if (!lu2p)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lu2p->reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(lu2p->reg)) {
		dev_err(&pdev->dev, "Failed to map register memory (phy)\n");
		return PTR_ERR(lu2p->reg);
	}

	lu2p->pci_sata_usb_ctrl_reg =
		syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "syscon");
	if (IS_ERR(lu2p->pci_sata_usb_ctrl_reg)) {
		dev_err(&pdev->dev, "Failed to get pci_sata_usb_ctrl syscon regmap\n");
		return PTR_ERR(lu2p->pci_sata_usb_ctrl_reg);
	}

	lu2p->clk = devm_clk_get(&pdev->dev, "usb");
	if (IS_ERR(lu2p->clk)) {
		dev_err(&pdev->dev, "Failed to get clock of phy controller\n");
		return PTR_ERR(lu2p->clk);
	}

	lu2p->rstc_phy = devm_reset_control_get(&pdev->dev, "phy");
	if (IS_ERR(lu2p->rstc_phy)) {
		dev_err(&pdev->dev, "Failed to get phy reset control\n");
		return PTR_ERR(lu2p->rstc_phy);
	}

	lu2p->rstc_utmi = devm_reset_control_get(&pdev->dev, "utmi");
	if (IS_ERR(lu2p->rstc_utmi)) {
		dev_err(&pdev->dev, "Failed to get utmi reset control\n");
		return PTR_ERR(lu2p->rstc_utmi);
	}

	lu2p->rstc_axi = devm_reset_control_get(&pdev->dev, "axi");
	if (IS_ERR(lu2p->rstc_axi)) {
		dev_err(&pdev->dev, "Failed to get axi reset control\n");
		return PTR_ERR(lu2p->rstc_axi);
	}

	lu2p->phy = devm_phy_create(&pdev->dev, NULL, &ls1024a_usb2_otg_phy_ops);
	if (IS_ERR(lu2p->phy)) {
		dev_err(&pdev->dev, "Failed to create phy\n");
		return PTR_ERR(lu2p->phy);
	}

	phy_set_drvdata(lu2p->phy, lu2p);

	phy_provider = devm_of_phy_provider_register(&pdev->dev,
						     of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static const struct of_device_id ls1024a_usb2_otg_phy_match[] = {
	{ .compatible = "fsl,ls1024a-usb2-phy", },
	{ },
};
MODULE_DEVICE_TABLE(of, ls1024a_usb2_otg_phy_match);

static struct platform_driver ls1024a_usb2_otg_phy_driver = {
	.probe		= ls1024a_usb2_otg_phy_probe,
	.driver		= {
		.name 	= "ls1024a-usb2-phy",
		.of_match_table = ls1024a_usb2_otg_phy_match,
	},
};
module_platform_driver(ls1024a_usb2_otg_phy_driver);

MODULE_DESCRIPTION("NXP QorIQ LS1024A USB2 PHY");
MODULE_AUTHOR("Daniel Mentz <danielmentz@google.com>");
MODULE_LICENSE("GPL v2");
