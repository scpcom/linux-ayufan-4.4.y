/*
 * Freescale LS1024A SoCs Reset Controller driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/reset-controller.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <dt-bindings/reset-controller/ls1024a-resets.h>

/**
 * Reset channel regmap configuration
 *
 * @reset: regmap field for the channel's reset bit.
 */
struct reset_channel {
	struct regmap_field *reset;
};

struct ls1024a_reset_data {
	spinlock_t			lock;
	struct regmap			*regmap;
	struct reset_channel		*channels;
	struct reset_controller_dev	rcdev;
};

#define DEVICE_RST_CNTRL		0x000
#define SERDES_RST_CNTRL		0x004
#define PCIe_SATA_RST_CNTRL		0x008
#define USB_RST_CNTRL			0x00C
#define AXI_RESET_0			0x050
#define AXI_RESET_1			0x054
#define AXI_RESET_2			0x058
#define A9DP_MPU_RESET			0x070
#define A9DP_CPU_RESET			0x078
#define A9DP_RESET			0x088
#define L2CC_RESET_CNTRL		0x098
#define TPI_RESET			0x0A8
#define CSYS_RESET			0x0B8
#define EXTPHY0_RESET			0x0C8
#define EXTPHY1_RESET			0x0D8
#define EXTPHY2_RESET			0x0E8
#define DDR_RESET			0x0F8
#define PFE_RESET			0x108
#define IPSEC_RESET			0x118
#define DECT_RESET_CNTRL		0x128
#define GEMTX_RESET_CNTRL		0x138
#define TDMNTG_RESET_CNTRL		0x148
#define TSUNTG_RESET 			0x158
#define SATA_PMU_RESET_CNTRL		0x168
#define SATA_OOB_RESET_CNTRL		0x178
#define SATA_OCC_RESET			0x188
#define PCIE_OCC_RESET			0x198
#define SGMII_OCC_RESET 		0x1A8

#define RESET_BIT(_rr, _rb)	REG_FIELD((_rr), (_rb), (_rb))

static const struct reg_field reset_bits[] = {
	[LS1024A_AXI_DPI_CIE_RESET]	= RESET_BIT(AXI_RESET_0, 5),
	[LS1024A_AXI_DPI_DECOMP_RESET]	= RESET_BIT(AXI_RESET_0, 6),
	[LS1024A_AXI_IPSEC_EAPE_RESET]	= RESET_BIT(AXI_RESET_1, 1),
	[LS1024A_AXI_IPSEC_SPACC_RESET]	= RESET_BIT(AXI_RESET_1, 2),
	[LS1024A_PFE_SYS_RESET]		= RESET_BIT(AXI_RESET_1, 3),
	[LS1024A_AXI_TDM_RESET]		= RESET_BIT(AXI_RESET_1, 4),
	[LS1024A_AXI_RTC_RESET]		= RESET_BIT(AXI_RESET_1, 7),
	[LS1024A_AXI_PCIE0_RESET]	= RESET_BIT(AXI_RESET_2, 0),
	[LS1024A_AXI_PCIE1_RESET]	= RESET_BIT(AXI_RESET_2, 1),
	[LS1024A_AXI_SATA_RESET]	= RESET_BIT(AXI_RESET_2, 2),
	[LS1024A_AXI_USB0_RESET]	= RESET_BIT(AXI_RESET_2, 3),
	[LS1024A_AXI_USB1_RESET]	= RESET_BIT(AXI_RESET_2, 4),
	[LS1024A_USB0_PHY_RESET]	= RESET_BIT(USB_RST_CNTRL, 0),
	[LS1024A_UTMI_USB0_RESET]	= RESET_BIT(USB_RST_CNTRL, 1),
	[LS1024A_USB1_PHY_RESET]	= RESET_BIT(USB_RST_CNTRL, 4),
	[LS1024A_UTMI_USB1_RESET]	= RESET_BIT(USB_RST_CNTRL, 5),
	[LS1024A_SERDES_PCIE0_RESET]	= REG_FIELD(PCIe_SATA_RST_CNTRL, 0, 1),
	[LS1024A_SERDES_PCIE1_RESET]	= REG_FIELD(PCIe_SATA_RST_CNTRL, 2, 3),
	[LS1024A_SERDES_SATA0_RESET]	= REG_FIELD(PCIe_SATA_RST_CNTRL, 4, 5),
	[LS1024A_SERDES_SATA1_RESET]	= REG_FIELD(PCIe_SATA_RST_CNTRL, 6, 7),
	[LS1024A_PFE_CORE_RESET]	= RESET_BIT(PFE_RESET, 0),
	[LS1024A_IPSEC_EAPE_CORE_RESET]	= RESET_BIT(IPSEC_RESET, 0),
	[LS1024A_GEMTX_RESET]		= RESET_BIT(GEMTX_RESET_CNTRL, 0),
	[LS1024A_DECT_RESET]		= RESET_BIT(DECT_RESET_CNTRL, 0),
	[LS1024A_DDR_CNTLR_RESET]	= RESET_BIT(DDR_RESET, 1),
	[LS1024A_DDR_PHY_RESET]		= RESET_BIT(DDR_RESET, 0),
	[LS1024A_SERDES0_RESET]		= RESET_BIT(SERDES_RST_CNTRL, 0),
	[LS1024A_SERDES1_RESET]		= RESET_BIT(SERDES_RST_CNTRL, 1),
	[LS1024A_SERDES2_RESET]		= RESET_BIT(SERDES_RST_CNTRL, 2),
	[LS1024A_SGMII_RESET]		= RESET_BIT(SGMII_OCC_RESET, 0),
	[LS1024A_SATA_PMU_RESET]	= RESET_BIT(SATA_PMU_RESET_CNTRL, 0),
	[LS1024A_SATA_OOB_RESET]	= RESET_BIT(SATA_OOB_RESET_CNTRL, 0),
	[LS1024A_TDMNTG_RESET]		= RESET_BIT(TDMNTG_RESET_CNTRL, 0),
};

#define to_ls1024a_reset_data(_rst) \
	container_of(_rst, struct ls1024a_reset_data, rcdev)

static int ls1024a_reset_program_hw(struct reset_controller_dev *rcdev,
				   unsigned long idx, int assert)
{
	struct ls1024a_reset_data *rst = to_ls1024a_reset_data(rcdev);
	const struct reset_channel *ch;
	u32 ctrl_val = assert ? 0xFFFFFFFF : 0;
	int err;

	if (idx >= rcdev->nr_resets)
		return -EINVAL;

	ch = &rst->channels[idx];

	err = regmap_field_write(ch->reset, ctrl_val);
	if (err)
		return err;

	return 0;
}

static int ls1024a_reset_assert(struct reset_controller_dev *rcdev,
			       unsigned long idx)
{
	return ls1024a_reset_program_hw(rcdev, idx, true);
}

static int ls1024a_reset_deassert(struct reset_controller_dev *rcdev,
				 unsigned long idx)
{
	return ls1024a_reset_program_hw(rcdev, idx, false);
}

static int ls1024a_reset_status(struct reset_controller_dev *rcdev,
				   unsigned long idx)
{
	struct ls1024a_reset_data *rst = to_ls1024a_reset_data(rcdev);
	const struct reset_channel *ch;
	unsigned cur_val;
	int err;

	if (idx >= rcdev->nr_resets)
		return -EINVAL;

	ch = &rst->channels[idx];

	err = regmap_field_read(ch->reset, &cur_val);
	if (err)
		return err;

	return cur_val;
}

static struct reset_control_ops ls1024a_reset_ops = {
	.assert		= ls1024a_reset_assert,
	.deassert	= ls1024a_reset_deassert,
	.status		= ls1024a_reset_status,
};

static int ls1024a_reset_probe(struct platform_device *pdev)
{
	struct ls1024a_reset_data *data;
	size_t size;
	int i;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	size = sizeof(struct reset_channel) * ARRAY_SIZE(reset_bits);

	data->channels = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
	if (!data->channels)
		return -ENOMEM;

	data->regmap = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "syscon");
	if (IS_ERR(data->regmap)) {
		dev_err(&pdev->dev,
			"Error retrieving reset syscon\n");
		return PTR_ERR(data->regmap);
	}
	spin_lock_init(&data->lock);

	data->rcdev.owner = THIS_MODULE;
	data->rcdev.nr_resets = ARRAY_SIZE(reset_bits);
	data->rcdev.ops = &ls1024a_reset_ops;
	data->rcdev.of_node = pdev->dev.of_node;

	for (i = 0; i < ARRAY_SIZE(reset_bits); i++) {
		struct regmap_field *f;
		const struct reg_field zero = {0};

		if (memcmp(&reset_bits[i], &zero, sizeof(zero))) {
			f = devm_regmap_field_alloc(&pdev->dev, data->regmap, reset_bits[i]);
			if (IS_ERR(f))
				return PTR_ERR(f);
			data->channels[i].reset = f;
		}

	}

	return reset_controller_register(&data->rcdev);
}

static const struct of_device_id ls1024a_reset_dt_ids[] = {
	 { .compatible = "fsl,ls1024a-reset", },
	 { /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ls1024a_reset_dt_ids);

static struct platform_driver ls1024a_reset_driver = {
	.probe	= ls1024a_reset_probe,
	.driver = {
		.name		= "ls1024a-reset",
		.of_match_table	= ls1024a_reset_dt_ids,
	},
};
static int __init ls1024a_reset_init(void)
{
	return platform_driver_register(&ls1024a_reset_driver);
}

arch_initcall(ls1024a_reset_init);

MODULE_DESCRIPTION("Freescale LS1024A SoCs Reset Controller Driver");
MODULE_LICENSE("GPL");
