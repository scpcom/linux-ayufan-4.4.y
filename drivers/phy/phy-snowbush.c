#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/sched.h>
#include <linux/reset.h>

#include <dt-bindings/phy/phy.h>

#include "phy-snowbush-regs.h"

struct snowbush_phy {
	struct phy *phy;
	void __iomem *base;
	void __iomem *dwc1_serdes_cfg_base;
	struct reset_control *reset;
	struct reset_control *pcie_rst;
	struct reset_control *sata_rst;
	bool pcie_tx_pol_inv;
	bool sata_tx_pol_inv;
	u32 sata_gen;
	u32 ctrlreg;
	u8 type;
};

struct snowbush_dev {
	struct device *dev;
	struct regmap *regmap;
	struct mutex sbphy_mutex;
	struct snowbush_phy **phys;
	int nphys;
};


#define SD_COMMON_LANE    0xA00

#define SD_PHY_STS_REG_OFST		0x00
#define SD_PHY_CTRL1_REG_OFST		0x04
#define SD_PHY_CTRL2_REG_OFST		0x08
#define SD_PHY_CTRL3_REG_OFST		0x0C

#define MAX_LANE_OK_WAIT_JIFFIES	(200 * HZ) / 1000    /* 200ms */
#define MAX_CMU_OK_WAIT_JIFFIES		(2000 * HZ) / 1000   /* 2 Seconds */

/**
 * This function Wait for the 'Lane OK' to be signaled by the
 * Snowbush Serdes PHY.
 * @param sbphy_num	SerDes PHY intefrace number.
 */
static int wait_lane_ok(struct snowbush_phy *sbphy_phy,
				    struct snowbush_dev *sbphy_dev)
{
	u32 rd_data = 0, masked_data = 0;
	u32 lane_ok_dtctd_mask = 0x00001000;
	unsigned long deadline = jiffies + MAX_LANE_OK_WAIT_JIFFIES;

	/* Keep looping until you see the lane_ok_o of Serdes */
	do
	{
		rd_data = readl(sbphy_phy->dwc1_serdes_cfg_base + SD_PHY_STS_REG_OFST);

		/* Mask lane_ok Status */
		masked_data = rd_data & lane_ok_dtctd_mask;

		if(masked_data == lane_ok_dtctd_mask) {
			/* Lane OK Detected on Serdes Port */
			dev_info(&sbphy_phy->phy->dev, "Serdes: Lane OK passed\n");
			return 1;
		}

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	dev_warn(&sbphy_phy->phy->dev, "Serdes: Lane OK failed\n");
	return 0;
}


/**
 * This function wait for the 'CMU OK' to be signaled by the
 * Snowbush Serdes PHY.
 * @param sbphy_num	SerDes PHY intefrace number.
 */
static int wait_cmu_ok(struct snowbush_phy *sbphy_phy,
				    struct snowbush_dev *sbphy_dev)
{
	u32 rd_data = 0, masked_data = 0;
	u32 cmu_ok_dtctd_mask = 0x00004000;
	volatile void __iomem *CMU_Offset;
	unsigned long deadline = jiffies + MAX_CMU_OK_WAIT_JIFFIES;

	CMU_Offset = sbphy_phy->dwc1_serdes_cfg_base + SD_PHY_STS_REG_OFST;

	/* Keep looping until you see the cmu_ok_o of Serdes */
	do
	{
		rd_data = readl(CMU_Offset);

		/* Mask cmu_ok Status */
		masked_data = rd_data & cmu_ok_dtctd_mask;

		if(masked_data == cmu_ok_dtctd_mask) {
			/* CMU OK Detected on Serdes Port */
			dev_info(&sbphy_phy->phy->dev, "Serdes: CMU OK Passed\n");
			return 1;
		}

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	dev_warn(&sbphy_phy->phy->dev, "Serdes: CMU OK failed\n");

	return 0;
}


/**
 * This function wait for the specified configured Snowbush PHY
 * (Serdes) to issue it's CMU-OK, and it's Lane to become Ready
 * after releasing the CMU & Lane resets.
 * @param sbphy_num	SerDes PHY intefrace number.
 */
static int wait_sb_cmu_lane_rdy(struct snowbush_phy *sbphy_phy,
				    struct snowbush_dev *sbphy_dev)
{
	volatile void __iomem *sd_ctl2_reg_offset;
	u32 cmu_rst_mask = 0x00010000;
	u32 lane_rst_mask = 0x00000040;
	u32 tmp = 0;

	sd_ctl2_reg_offset =
		sbphy_phy->dwc1_serdes_cfg_base + SD_PHY_CTRL2_REG_OFST;

	/* Releasing the CMU Reset */
	tmp = readl(sd_ctl2_reg_offset);
	tmp = tmp & (~cmu_rst_mask);
	tmp = tmp | cmu_rst_mask;

	writel(tmp, sd_ctl2_reg_offset );

	/* Waiting for CMU OK */
	if( !wait_cmu_ok(sbphy_phy, sbphy_dev) )
		return -1;

	if (sbphy_phy->type == PHY_TYPE_PCIE)
		writel(0xC3, sbphy_phy->base + (SD_COMMON_LANE << 2));
	else
		writel(0x03, sbphy_phy->base + (SD_COMMON_LANE << 2));

	/* Releasing the Lane Reset */
	tmp = readl(sd_ctl2_reg_offset);
	tmp = tmp & (~lane_rst_mask);
	tmp = tmp | lane_rst_mask;

	writel(tmp, sd_ctl2_reg_offset);

	/* Waiting for the Lane Ready */
	if (sbphy_phy->type != PHY_TYPE_PCIE) {
		if( !wait_lane_ok(sbphy_phy, sbphy_dev) )
			return -1;
	}

	return 0;
}


/**
 * This function initialize the Snowbush PHY (Serdes) for operation
 * with the one of the PCIE,SATA or SGMII IP blocks, and then waiting
 * until it issue it's CMU-OK, and it's  Lane to become Ready after
 * releasing the CMU & Lane Resets.
 * @param phy_num	SerDes PHY intefrace number.
 * @param *regs		Register file (Array of registers and coresponding
 *                      values to be programmed).
 * @param size		Number of registers to be programmed.
 */
static int serdes_phy_init(struct snowbush_phy *sbphy_phy,
			    struct snowbush_dev *sbphy_dev)
{
	int i, size, ret;
	struct serdes_regs_s *regs;

	static int misc = 0;
	ret = reset_control_deassert(sbphy_phy->reset);
	if (ret)
		return ret;

	if (sbphy_phy->type == PHY_TYPE_PCIE && sbphy_phy->pcie_rst) {
		ret = reset_control_deassert(sbphy_phy->pcie_rst);
		if (ret)
			return ret;
	}
	if (sbphy_phy->type == PHY_TYPE_SATA && sbphy_phy->sata_rst) {
		ret = reset_control_deassert(sbphy_phy->sata_rst);
		if (ret)
			return ret;
	}
	if (!misc) {
		/* Move SATA controller to DDRC2 port */
		writel(readl((void*) 0xF097006C) | 0x2, (void*) 0xF097006C); // TODO:COMCERTO_GPIO_FABRIC_CTRL_REG

		writel(readl((void*) 0xF09B0058) & ~4, (void*) 0xF09B0058); // AXI_RESET_2 TODO: SATA_AXI

		writel(readl((void*) 0xF09B0048) | 3, (void*) 0xF09B0048); // AXI_CLK_CNTRL_2
		writel(readl((void*) 0xF09B0048) | 4, (void*) 0xF09B0048); // AXI_CLK_CNTRL_2: SATA

		writel(readl((void*) 0xF09B0168) & ~1, (void*) 0xF09B0168); // TODO: SATA_PMU
		writel(readl((void*) 0xF09B0178) & ~1, (void*) 0xF09B0178); // TODO: SATA_OOB

		misc = 1;
	}


	if (sbphy_phy->type == PHY_TYPE_PCIE) {
		/* SW select for ck_soc_div_i SOC clock */
		writel(0xFF3C, sbphy_phy->dwc1_serdes_cfg_base + SD_PHY_CTRL3_REG_OFST);
		writel(readl(sbphy_phy->dwc1_serdes_cfg_base + SD_PHY_CTRL2_REG_OFST) & ~0x3,
				sbphy_phy->dwc1_serdes_cfg_base + SD_PHY_CTRL2_REG_OFST);
	}

	if (sbphy_phy->type == PHY_TYPE_SATA) {
		regs = sata_phy_reg_file_24; /* TODO: Support for 48MHz crystal */
		size = ARRAY_SIZE(sata_phy_reg_file_24);
	} else if (sbphy_phy->type == PHY_TYPE_PCIE) {
		regs = pcie_phy_reg_file_24;
		size = ARRAY_SIZE(pcie_phy_reg_file_24);
		if (1) { // (system_rev == 1) { /* TODO */
			// C2K RevA1 devices use a different serdes clk divider
			regs[0x61].val = 0x6;
		}

	} else {
		BUG();
	}

	/* Initilize serdes phy registers */
	for(i = 0; i < size; i++ )
		writel(regs[i].val, sbphy_phy->base + regs[i].ofst);

	/* Wait for the initialization of Serdes-1 Port/Lane to become Ready */
	return wait_sb_cmu_lane_rdy(sbphy_phy, sbphy_dev);
}

static int snowbush_init(struct phy *phy)
{
	struct snowbush_phy *sbphy_phy = phy_get_drvdata(phy);
	struct snowbush_dev *sbphy_dev = dev_get_drvdata(phy->dev.parent);
	int ret = 0;

	mutex_lock(&sbphy_dev->sbphy_mutex);
	serdes_phy_init(sbphy_phy, sbphy_dev);
	mutex_unlock(&sbphy_dev->sbphy_mutex);

	return ret;
}

static struct phy *snowbush_xlate(struct device *dev,
				   struct of_phandle_args *args)
{
	struct snowbush_dev *sbphy_dev = dev_get_drvdata(dev);
	struct snowbush_phy *sbphy_phy = NULL;
	struct device_node *phynode = args->np;
	int index;

	if (!of_device_is_available(phynode)) {
		dev_warn(dev, "Requested PHY is disabled\n");
		return ERR_PTR(-ENODEV);
	}

	if (args->args_count != 1) {
		dev_err(dev, "Invalid number of cells in 'phy' property\n");
		return ERR_PTR(-EINVAL);
	}

	for (index = 0; index < sbphy_dev->nphys; index++)
		if (phynode == sbphy_dev->phys[index]->phy->dev.of_node) {
			sbphy_phy = sbphy_dev->phys[index];
			break;
		}

	if (!sbphy_phy) {
		dev_err(dev, "Failed to find appropriate phy\n");
		return ERR_PTR(-EINVAL);
	}

	sbphy_phy->type = args->args[0];

	if (!(sbphy_phy->type == PHY_TYPE_SATA ||
	      sbphy_phy->type == PHY_TYPE_PCIE)) {
		dev_err(dev, "Unsupported device type: %d\n", sbphy_phy->type);
		return ERR_PTR(-EINVAL);
	}

	return sbphy_phy->phy;
}

static struct phy_ops snowbush_ops = {
	.init		= snowbush_init,
	.owner		= THIS_MODULE,
};

static int snowbush_of_probe(struct device_node *phynode,
			      struct snowbush_phy *sbphy_phy)
{
	sbphy_phy->base = of_iomap(phynode, 0);
	if (!sbphy_phy->base) {
		dev_err(&sbphy_phy->phy->dev, "Failed to map %s\n", phynode->full_name);
		return -EINVAL;
	}
	sbphy_phy->dwc1_serdes_cfg_base = of_iomap(phynode, 1);
	if (!sbphy_phy->dwc1_serdes_cfg_base) {
		dev_err(&sbphy_phy->phy->dev, "Failed to map %s dwc1_serdes_cfg_base\n", phynode->full_name);
		return -EINVAL;
	}
	sbphy_phy->reset = devm_reset_control_get(&sbphy_phy->phy->dev, "serdes");
	if (IS_ERR(sbphy_phy->reset)) {
		dev_err(&sbphy_phy->phy->dev, "failed to get reset control\n");
		return PTR_ERR(sbphy_phy->reset);
	}
	sbphy_phy->pcie_rst = devm_reset_control_get(&sbphy_phy->phy->dev, "serdes_pcie");
	if (IS_ERR(sbphy_phy->pcie_rst)) {
		sbphy_phy->pcie_rst = 0;
	}
	sbphy_phy->sata_rst = devm_reset_control_get(&sbphy_phy->phy->dev, "serdes_sata");
	if (IS_ERR(sbphy_phy->sata_rst)) {
		sbphy_phy->sata_rst = 0;
	}
	/* TODO */
	return 0;
}

static int snowbush_probe(struct platform_device *pdev)
{
	struct device_node *child, *np = pdev->dev.of_node;
	struct snowbush_dev *sbphy_dev;
	struct phy_provider *provider;
	struct phy *phy;
	int ret, port = 0;

	sbphy_dev = devm_kzalloc(&pdev->dev, sizeof(*sbphy_dev), GFP_KERNEL);
	if (!sbphy_dev)
		return -ENOMEM;

	sbphy_dev->nphys = of_get_child_count(np);
	sbphy_dev->phys = devm_kcalloc(&pdev->dev, sbphy_dev->nphys,
				       sizeof(*sbphy_dev->phys), GFP_KERNEL);
	if (!sbphy_dev->phys)
		return -ENOMEM;

	sbphy_dev->dev = &pdev->dev;

	dev_set_drvdata(&pdev->dev, sbphy_dev);

	mutex_init(&sbphy_dev->sbphy_mutex);

	for_each_child_of_node(np, child) {
		struct snowbush_phy *sbphy_phy;

		sbphy_phy = devm_kzalloc(&pdev->dev, sizeof(*sbphy_phy),
					 GFP_KERNEL);
		if (!sbphy_phy)
			return -ENOMEM;

		sbphy_dev->phys[port] = sbphy_phy;

		phy = devm_phy_create(&pdev->dev, child, &snowbush_ops);
		if (IS_ERR(phy)) {
			dev_err(&pdev->dev, "failed to create PHY\n");
			return PTR_ERR(phy);
		}

		sbphy_dev->phys[port]->phy = phy;

		ret = snowbush_of_probe(child, sbphy_phy);
		if (ret)
			return ret;

		phy_set_drvdata(phy, sbphy_dev->phys[port]);

		port++;
	}

	provider = devm_of_phy_provider_register(&pdev->dev, snowbush_xlate);
	return PTR_ERR_OR_ZERO(provider);
}

static const struct of_device_id snowbush_of_match[] = {
	{ .compatible = "fsl,ls1024a-snowbush-phy", },
	{ },
};
MODULE_DEVICE_TABLE(of, snowbush_of_match);

static struct platform_driver snowbush_driver = {
	.probe	= snowbush_probe,
	.driver = {
		.name	= "snowbush-phy",
		.of_match_table	= snowbush_of_match,
	}
};
module_platform_driver(snowbush_driver);

MODULE_AUTHOR("Daniel Mentz <danielmentz@google.com>");
MODULE_DESCRIPTION("Snowbush PHY driver");
MODULE_LICENSE("GPL v2");
