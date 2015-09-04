/*
 * This file is licensed under the terms of the GNU General Public
 * License version 2 or later. This program is licensed "as is"
 * without any warranty of any kind, whether express or implied.
 */

#include <dt-bindings/phy/phy.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

struct ls1024a_usb3_phy {
	struct phy *phy;
	void __iomem *reg;
	struct reset_control *rstc;
};

static int ls1024a_usb_phy_init(struct phy *phy)
{
	struct ls1024a_usb3_phy *lu_phy;
	u32 val;

	lu_phy = phy_get_drvdata(phy);
	if (!lu_phy)
		return -ENODEV;

	if (!reset_control_status(lu_phy->rstc)) {
		reset_control_assert(lu_phy->rstc);
		udelay(1000);
	}

        writel(0x00E00080, lu_phy->reg + 0x10);

	/* TODO: Support 48 MHz refclock and external clock */
#if 0
	//Configuration for internal clock
	if(usb3_clk_internal)
	{
		printk(KERN_INFO "USB3.0 clock selected: internal\n", __func__);

		if(HAL_get_ref_clk() == REF_CLK_24MHZ)
#endif
			val = 0x420E82A8;
#if 0
		else
			val = 0x420E82A9;
	}
	else
	{
		val = 0x4209927A;
		printk(KERN_INFO "USB3.0 clock selected: external\n", __func__);
	}
#endif

	writel(val, lu_phy->reg + 0x20);
        writel(0x69C34F53, lu_phy->reg + 0x24);
        writel(0x0005D815, lu_phy->reg + 0x28);
        writel(0x00000801, lu_phy->reg + 0x2C);

	reset_control_deassert(lu_phy->rstc);
	udelay(1000);

	return 0;
}

static struct phy_ops ls1024a_usb_phy_ops = {
	.init = ls1024a_usb_phy_init,
	.owner = THIS_MODULE,
};

static int ls1024a_usb_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy *phy;
	struct phy_provider *phy_provider;
	void __iomem *reg;
	struct reset_control *rstc;
	struct resource *res;
	struct ls1024a_usb3_phy *lu_phy;

	lu_phy = devm_kzalloc(dev, sizeof(*lu_phy), GFP_KERNEL);
	if (!lu_phy)
		return  -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	rstc = devm_reset_control_get(&pdev->dev, "phy");
	if (IS_ERR(rstc))
		return PTR_ERR(rstc);

	phy = devm_phy_create(dev, NULL, &ls1024a_usb_phy_ops);
	if (IS_ERR(phy))
		return PTR_ERR(phy);

	lu_phy->phy = phy;
	lu_phy->reg = reg;
	lu_phy->rstc = rstc;

	dev_set_drvdata(dev, lu_phy);
	phy_set_drvdata(phy, lu_phy);

	phy_provider = devm_of_phy_provider_register(&pdev->dev,
						of_phy_simple_xlate);
	return PTR_ERR_OR_ZERO(phy_provider);
}

static const struct of_device_id phy_of_match[] = {
	{ .compatible = "fsl,ls1024a-usb3-phy", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, phy_of_match);

static struct platform_driver ls1024a_usb_phy_driver = {
	.probe	= ls1024a_usb_phy_probe,
	.driver = {
		.of_match_table	= phy_of_match,
		.name  = "ls1024a-usb3-phy",
		.owner = THIS_MODULE,
	}
};
module_platform_driver(ls1024a_usb_phy_driver);

MODULE_DESCRIPTION("Freescale QorIQ LS1024A USB3 PHY");
MODULE_AUTHOR("Daniel Mentz <danielmentz@google.com>");
MODULE_LICENSE("GPL v2");
