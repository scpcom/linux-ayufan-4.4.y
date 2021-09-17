#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mbus.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/io.h>
#include "mvCommon.h"
#include "mvOs.h"
#include "voiceband/tdm/mvTdm.h"
#include "voiceband/mvSysTdmSpi.h"

#define DRV_NAME "mvebu_phone"

int tdm_base;
int use_pclk_external;
int mv_phone_enabled;
struct mv_phone_dev *priv;

int mvSysTdmInit(MV_TDM_PARAMS *tdm_params)
{
	MV_TDM_HAL_DATA hal_data;
	MV_TDM_UNIT_TYPE tdm_unit;
	u8 spi_mode = 0;
	int ret;

	tdm_unit = mvCtrlTdmUnitTypeGet();
	if (tdm_unit != MV_TDM_UNIT_TDM2C)
		return 0;

	hal_data.spiMode = spi_mode;

	switch (priv->pclk_freq_mhz) {
	case 8:
		hal_data.frameTs = MV_FRAME_128TS;
		break;
	case 4:
		hal_data.frameTs = MV_FRAME_64TS;
		break;
	case 2:
		hal_data.frameTs = MV_FRAME_32TS;
		break;
	default:
		hal_data.frameTs = MV_FRAME_128TS;
		break;
	}

	ret = mvTdmHalInit(tdm_params, &hal_data);

	priv->tdm_params = tdm_params;

	return ret;
}

void mvSysTdmIntEnable(u8 dev_id)
{
	MV_TDM_UNIT_TYPE tdmUnit;
	tdmUnit = mvCtrlTdmUnitTypeGet();

	if (tdmUnit == MV_TDM_UNIT_TDM2C)
		mvTdmIntEnable();
}

void mvSysTdmIntDisable(u8 dev_id)
{
	MV_TDM_UNIT_TYPE tdm_unit;
	tdm_unit = mvCtrlTdmUnitTypeGet();

	if (tdm_unit == MV_TDM_UNIT_TDM2C)
		mvTdmIntDisable();
}

u32 mvBoardSlicUnitTypeGet(void)
{
	return MV_BOARD_SLIC_DISABLED;
}

u32 mvCtrlTdmUnitIrqGet(void)
{
	return priv->irq;
}

MV_TDM_UNIT_TYPE mvCtrlTdmUnitTypeGet(void)
{
	MV_TDM_UNIT_TYPE tdm_type = MV_TDM_UNIT_NONE;

	if (!mv_phone_enabled)
		return tdm_type;

	if (of_device_is_compatible(priv->np, "marvell,armada-380-tdm"))
		tdm_type = MV_TDM_UNIT_TDM2C;

	return tdm_type;
}

static int mv_phone_tdm_clk_pll_config(struct platform_device *pdev)
{
	struct resource *mem;
	u32 reg_val;
	u16 freq_offset = 0x22b0;
	u8 tdm_postdiv = 0x6, fb_clk_div = 0x1d;

	if (!priv->pll_base) {
		mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   "pll_regs");
		priv->pll_base = devm_ioremap_resource(&pdev->dev, mem);
		if (IS_ERR(priv->pll_base))
			return -ENOMEM;
	}

	reg_val = readl(priv->pll_base + TDM_PLL_CONF_REG1);
	reg_val &= ~TDM_PLL_FREQ_OFFSET_VALID;
	reg_val &= ~TDM_PLL_SW_RESET;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG1);

	udelay(1);

	reg_val = readl(priv->pll_base + TDM_PLL_CONF_REG0);
	reg_val &= ~TDM_PLL_FB_CLK_DIV_MASK;
	reg_val |= (fb_clk_div << TDM_PLL_FB_CLK_DIV_OFFSET);
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG0);

	reg_val = readl(priv->pll_base + TDM_PLL_CONF_REG2);
	reg_val &= ~TDM_PLL_POSTDIV_MASK;
	reg_val |= tdm_postdiv;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG2);

	reg_val = readl(priv->pll_base + TDM_PLL_CONF_REG1);
	reg_val &= ~TDM_PLL_FREQ_OFFSET_MASK;
	reg_val |= freq_offset;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG1);

	udelay(1);

	reg_val |= TDM_PLL_SW_RESET;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG1);

	udelay(50);

	reg_val |= TDM_PLL_FREQ_OFFSET_VALID;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG1);

	return 0;
}

static int mv_phone_dco_post_div_config(struct platform_device *pdev,
					u32 pclk_freq_mhz)
{
	struct resource *mem;
	u32 reg_val, pcm_clk_ratio;

	if (!priv->dco_div_reg) {
		mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   "dco_div");
		priv->dco_div_reg = devm_ioremap_resource(&pdev->dev, mem);
		if (IS_ERR(priv->dco_div_reg))
			return -ENOMEM;
	}

	switch (pclk_freq_mhz) {
	case 8:
		pcm_clk_ratio = DCO_CLK_DIV_RATIO_8M;
		break;
	case 4:
		pcm_clk_ratio = DCO_CLK_DIV_RATIO_4M;
		break;
	case 2:
		pcm_clk_ratio = DCO_CLK_DIV_RATIO_2M;
		break;
	default:
		pcm_clk_ratio = DCO_CLK_DIV_RATIO_8M;
		break;
	}

	reg_val = readl(priv->dco_div_reg);
	writel(MV_BIT_CLEAR(reg_val, DCO_CLK_DIV_RESET_OFFS),
	       priv->dco_div_reg);

	reg_val = readl(priv->dco_div_reg);
	writel((reg_val & ~DCO_CLK_DIV_RATIO_MASK) | pcm_clk_ratio,
	       priv->dco_div_reg);

	reg_val = readl(priv->dco_div_reg);
	writel(MV_BIT_SET(reg_val, DCO_CLK_DIV_MOD_OFFS), priv->dco_div_reg);
	mdelay(1);

	reg_val = readl(priv->dco_div_reg);
	writel(MV_BIT_CLEAR(reg_val, DCO_CLK_DIV_MOD_OFFS), priv->dco_div_reg);
	mdelay(1);

	reg_val = readl(priv->dco_div_reg);
	writel(MV_BIT_SET(reg_val, DCO_CLK_DIV_RESET_OFFS), priv->dco_div_reg);

	return 0;
}

static int mv_conf_mbus_windows(struct device *dev, void __iomem *regs,
				const struct mbus_dram_target_info *dram)
{
	int i;

	if (!dram) {
		dev_err(dev, "no mbus dram info\n");
		return -EINVAL;
	}

	for (i = 0; i < TDM_MBUS_MAX_WIN; i++) {
		writel(0, regs + TDM_WIN_CTRL_REG(i));
		writel(0, regs + TDM_WIN_BASE_REG(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		writel(((cs->size - 1) & 0xffff0000) |
			(cs->mbus_attr << 8) |
			(dram->mbus_dram_target_id << 4) | 1,
			regs + TDM_WIN_CTRL_REG(i));
		 
		writel(cs->base, regs + TDM_WIN_BASE_REG(i));
	}

	return 0;
}

static int mvebu_phone_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *mem;
	int err;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct mv_phone_dev),
			    GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "allocation failed\n");
		return -ENOMEM;
	}

	priv->np = np;

	mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tdm_regs");
	priv->tdm_base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(priv->tdm_base))
		return PTR_ERR(priv->tdm_base);
	tdm_base = (int)priv->tdm_base;

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq <= 0) {
		dev_err(&pdev->dev, "platform_get_irq failed\n");
		return -ENXIO;
	}

	err = mv_conf_mbus_windows(&pdev->dev, priv->tdm_base,
				   mv_mbus_dram_info());
	if (err < 0)
		return err;

	priv->clk = devm_clk_get(&pdev->dev, "gateclk");
	if (IS_ERR(priv->clk)) {
		dev_err(&pdev->dev, "no clock\n");
		return PTR_ERR(priv->clk);
	}

	err = clk_prepare_enable(priv->clk);
	if (err < 0)
		return err;

	if (of_property_read_bool(np, "use-external-pclk")) {
		dev_info(&pdev->dev, "using external pclk\n");
		use_pclk_external = 1;
	} else {
		dev_info(&pdev->dev, "using internal pclk\n");
		use_pclk_external = 0;
	}

	if (of_property_read_u32(np, "pclk-freq-mhz", &priv->pclk_freq_mhz) ||
	    (priv->pclk_freq_mhz != 8 && priv->pclk_freq_mhz != 4 &&
	     priv->pclk_freq_mhz != 2)) {
		priv->pclk_freq_mhz = 8;
		dev_info(&pdev->dev, "wrong pclk frequency in the DT\n");
	}
	dev_info(&pdev->dev, "setting pclk frequency to %d MHz\n",
		 priv->pclk_freq_mhz);

	if (of_device_is_compatible(np, "marvell,armada-380-tdm")) {
		err = mv_phone_tdm_clk_pll_config(pdev);
		err |= mv_phone_dco_post_div_config(pdev, priv->pclk_freq_mhz);
		if (err < 0)
			goto err_clk;
	}

	mv_phone_enabled = 1;

	return 0;

err_clk:
	clk_disable_unprepare(priv->clk);

	return err;
}

static int mvebu_phone_remove(struct platform_device *pdev)
{
	clk_disable_unprepare(priv->clk);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mvebu_phone_suspend(struct device *dev)
{
#if defined(MY_ABC_HERE)
	int i;

	for (i = 0; i < TDM_CTRL_REGS_NUM; i++)
		priv->tdm_ctrl_regs[i] = readl(priv->tdm_base + i);

	for (i = 0; i < TDM_SPI_REGS_NUM; i++)
		priv->tdm_spi_regs[i] = readl(priv->tdm_base +
					      TDM_SPI_REGS_OFFSET + i);

	priv->tdm_spi_mux_reg = MV_REG_READ(TDM_SPI_MUX_REG);
	priv->tdm_mbus_config_reg = MV_REG_READ(TDM_MBUS_CONFIG_REG);
	priv->tdm_misc_reg = MV_REG_READ(TDM_MISC_REG);
#endif  

	return 0;
}

static int mvebu_phone_resume(struct device *dev)
{
	struct platform_device *pdev = priv->parent;
#if defined(MY_ABC_HERE)
	int err, i;
#else
	int err;
#endif  

	err = mv_conf_mbus_windows(dev, priv->tdm_base,
				   mv_mbus_dram_info());
	if (err < 0)
		return err;

	if (of_device_is_compatible(priv->np, "marvell,armada-380-tdm")) {
		err = mv_phone_tdm_clk_pll_config(pdev);
		err |= mv_phone_dco_post_div_config(pdev, priv->pclk_freq_mhz);
		if (err < 0)
			return err;
	}

#if defined(MY_ABC_HERE)
	for (i = 0; i < TDM_CTRL_REGS_NUM; i++)
		writel(priv->tdm_ctrl_regs[i], priv->tdm_base + i);

	for (i = 0; i < TDM_SPI_REGS_NUM; i++)
		writel(priv->tdm_spi_regs[i], priv->tdm_base +
					      TDM_SPI_REGS_OFFSET + i);

	MV_REG_WRITE(TDM_SPI_MUX_REG, priv->tdm_spi_mux_reg);
	MV_REG_WRITE(TDM_MBUS_CONFIG_REG, priv->tdm_mbus_config_reg);
	MV_REG_WRITE(TDM_MISC_REG, priv->tdm_misc_reg);
#else
	mvSysTdmInit(priv->tdm_params);
#endif  

	return 0;
}
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops mvebu_phone_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(mvebu_phone_suspend, mvebu_phone_resume)
};

#define MVEBU_PHONE_PMOPS (&mvebu_phone_pmops)

#else
#define MVEBU_PHONE_PMOPS NULL
#endif

static const struct of_device_id mvebu_phone_match[] = {
	{ .compatible = "marvell,armada-380-tdm" },
	{ }
};
MODULE_DEVICE_TABLE(of, mvebu_phone_match);

static struct platform_driver mvebu_phone_driver = {
	.probe	= mvebu_phone_probe,
	.remove	= mvebu_phone_remove,
	.driver	= {
		.name	= DRV_NAME,
		.of_match_table = mvebu_phone_match,
		.owner	= THIS_MODULE,
		.pm	= MVEBU_PHONE_PMOPS,
	},
};

module_platform_driver(mvebu_phone_driver);

MODULE_DESCRIPTION("Marvell Telephony Driver");
MODULE_AUTHOR("Marcin Wojtas <mw@semihalf.com>");
