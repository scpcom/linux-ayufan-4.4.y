#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/thermal.h>
#if defined(MY_ABC_HERE)
#include <linux/interrupt.h>
#endif  

#if defined(MY_ABC_HERE)
#define THERMAL_VALID_MASK		0x1
#if defined(MY_ABC_HERE)
#define MCELSIUS(temp)            ((temp) * 1000)
#define CELSIUS(temp)         ((temp) / 1000)
#endif  
#else  
#define THERMAL_VALID_OFFSET		9
#define THERMAL_VALID_MASK		0x1
#define THERMAL_TEMP_OFFSET		10
#define THERMAL_TEMP_MASK		0x1ff
#endif  

#define PMU_TDC0_SW_RST_MASK		(0x1 << 1)
#define PMU_TM_DISABLE_OFFS		0
#define PMU_TM_DISABLE_MASK		(0x1 << PMU_TM_DISABLE_OFFS)
#define PMU_TDC0_REF_CAL_CNT_OFFS	11
#define PMU_TDC0_REF_CAL_CNT_MASK	(0x1ff << PMU_TDC0_REF_CAL_CNT_OFFS)
#define PMU_TDC0_OTF_CAL_MASK		(0x1 << 30)
#define PMU_TDC0_START_CAL_MASK		(0x1 << 25)

#if defined(MY_ABC_HERE)
#if defined(MY_ABC_HERE)
 
#else
#define A375_Z1_CAL_RESET_LSB          0x8011e214
#define A375_Z1_CAL_RESET_MSB          0x30a88019
#define A375_Z1_WORKAROUND_BIT         BIT(9)
#endif  
#define A375_UNIT_CONTROL_SHIFT		27
#define A375_UNIT_CONTROL_MASK		0x7
#define A375_READOUT_INVERT		BIT(15)
#define A375_HW_RESETn			BIT(8)
#define A380_HW_RESET			BIT(8)
#if defined(MY_ABC_HERE)
#define A380_CONTROL_MSB_OFFSET       4
#define A380_TSEN_TC_TRIM_MASK        0x7

#define A380_THRESH_DEFAULT_TEMP  100
#define A380_THRESH_DEFAULT_HYST  2
#define A380_THRESH_OFFSET        16
#define A380_THRESH_HYST_MASK     0x3
#define A380_THRESH_HYST_OFFSET       26
#endif  
struct armada_thermal_data;
#else  
struct armada_thermal_ops;
#endif  

struct armada_thermal_priv {
	void __iomem *sensor;
	void __iomem *control;
#if defined(MY_ABC_HERE)
	void __iomem *dfx;
#endif  
#if defined(MY_ABC_HERE)
	struct armada_thermal_data *data;
#if defined(MY_ABC_HERE)
	struct platform_device *pdev;
#endif  
#else  
	struct armada_thermal_ops *ops;
#endif  
};

#if defined(MY_ABC_HERE)
struct armada_thermal_data {
	 
	void (*init_sensor)(struct platform_device *pdev,
			    struct armada_thermal_priv *);

	bool (*is_valid)(struct armada_thermal_priv *);

	unsigned long coef_b;
	unsigned long coef_m;
	unsigned long coef_div;
	bool inverted;

	unsigned int temp_shift;
	unsigned int temp_mask;
	unsigned int is_valid_shift;
};
#if defined(MY_ABC_HERE)
inline unsigned int armada380_thresh_val_calc(unsigned int celsius_temp,
					      struct armada_thermal_data *data)
{
	int thresh_val;
	thresh_val = ((MCELSIUS(celsius_temp) * data->coef_div) +
		      data->coef_b) / data->coef_m;
	return thresh_val & data->temp_mask;
}
inline unsigned int armada380_thresh_celsius_calc(int thresh_val,
						  int hyst,
						  struct armada_thermal_data *data)
{
	unsigned int mcelsius_temp;
	mcelsius_temp = (((data->coef_m * (thresh_val + hyst)) -
			  data->coef_b) / data->coef_div);
	return CELSIUS(mcelsius_temp);
}
static void armada380_temp_set_threshold(struct platform_device *pdev,
					 struct armada_thermal_priv *priv)
{
	int temp, reg, hyst;
	unsigned int thresh;
	struct armada_thermal_data *data = priv->data;
	struct device_node *np = pdev->dev.of_node;
	 
	if (of_property_read_u32(np, "threshold", &thresh)) {
		thresh = A380_THRESH_DEFAULT_TEMP;
		dev_warn(&pdev->dev, "no threshold in DT, using default\n");
	}
	 
	if (of_property_read_u32(np, "hysteresis", &hyst)) {
		hyst = A380_THRESH_DEFAULT_HYST;
		dev_warn(&pdev->dev, "no hysteresis in DT, using default\n");
	}
	temp = armada380_thresh_val_calc(thresh, data);
	reg = readl_relaxed(priv->control + A380_CONTROL_MSB_OFFSET);
	 
	reg &= ~(data->temp_mask << A380_THRESH_OFFSET);
	reg |= (temp << A380_THRESH_OFFSET);
	 
	reg &= ~(A380_THRESH_HYST_MASK << A380_THRESH_HYST_OFFSET);
	reg |= (hyst << A380_THRESH_HYST_OFFSET);
	writel(reg, priv->control + A380_CONTROL_MSB_OFFSET);
	 
	hyst = 1 << (hyst + 2);
	dev_info(&pdev->dev, "Overheat threshold between %d..%d\n",
		armada380_thresh_celsius_calc(temp, -hyst, data),
		armada380_thresh_celsius_calc(temp, hyst, data));
}
#endif  
#else  
struct armada_thermal_ops {
	 
	void (*init_sensor)(struct armada_thermal_priv *);

	bool (*is_valid)(struct armada_thermal_priv *);
};
#endif  

#if defined(MY_ABC_HERE)
static struct thermal_zone_device* g_syno_tz = NULL;
static int armada_get_temp(struct thermal_zone_device *thermal,
			  unsigned long *temp);

unsigned long int syno_armada_get_temperature(int *temperature)
{
	const unsigned long int coef_b = 1169498786UL;
	const unsigned long int coef_m = 2000000UL;
	const unsigned long int coef_div = 4289;
	const unsigned long int measurement_offset = 24;   
	const unsigned long int lowest_report_value = 40;

	int ret = -EIO;
	unsigned long ulTemperature = 0;

	if (!g_syno_tz)
		return ret;

	ret = armada_get_temp(g_syno_tz, &ulTemperature);
	if (ret != 0)
		return ret;

	ulTemperature = ((ulTemperature * coef_div) + coef_b) / coef_m;
	ulTemperature = ((((10000 * ulTemperature) / 21445) * 1000) - 272674) / 1000;

	if (ulTemperature >= lowest_report_value + measurement_offset) {
		ulTemperature -= measurement_offset;
	} else {
		ulTemperature = lowest_report_value;
	}

	if (ulTemperature != (unsigned long)(int)ulTemperature)
		return -ERANGE;
	*temperature = (int)ulTemperature;
	return 0;
}

EXPORT_SYMBOL(syno_armada_get_temperature);
#endif  

#if defined(MY_ABC_HERE)
static void armadaxp_init_sensor(struct platform_device *pdev,
				 struct armada_thermal_priv *priv)
#else  
static void armadaxp_init_sensor(struct armada_thermal_priv *priv)
#endif  
{
	unsigned long reg;

	reg = readl_relaxed(priv->control);
	reg |= PMU_TDC0_OTF_CAL_MASK;
	writel(reg, priv->control);

	reg &= ~PMU_TDC0_REF_CAL_CNT_MASK;
	reg |= (0xf1 << PMU_TDC0_REF_CAL_CNT_OFFS);
	writel(reg, priv->control);

	reg = readl_relaxed(priv->control);
	writel((reg | PMU_TDC0_SW_RST_MASK), priv->control);

	writel(reg, priv->control);

	reg = readl_relaxed(priv->sensor);
	reg &= ~PMU_TM_DISABLE_MASK;
	writel(reg, priv->sensor);
}

#if defined(MY_ABC_HERE)
static void armada370_init_sensor(struct platform_device *pdev,
				  struct armada_thermal_priv *priv)
#else  
static void armada370_init_sensor(struct armada_thermal_priv *priv)
#endif  
{
	unsigned long reg;

	reg = readl_relaxed(priv->control);
	reg |= PMU_TDC0_OTF_CAL_MASK;
	writel(reg, priv->control);

	reg &= ~PMU_TDC0_REF_CAL_CNT_MASK;
	reg |= (0xf1 << PMU_TDC0_REF_CAL_CNT_OFFS);
	writel(reg, priv->control);

	reg &= ~PMU_TDC0_START_CAL_MASK;
	writel(reg, priv->control);

	mdelay(10);
}

#if defined(MY_ABC_HERE)
static void armada375_init_sensor(struct platform_device *pdev,
				  struct armada_thermal_priv *priv)
{
	unsigned long reg;
#if defined(MY_ABC_HERE)
	 
#else
	bool quirk_needed =
		!!of_device_is_compatible(pdev->dev.of_node,
				"marvell,armada375-z1-thermal");

	if (quirk_needed) {
		 
		writel(A375_Z1_CAL_RESET_LSB, priv->control);
		writel(A375_Z1_CAL_RESET_MSB, priv->control + 0x4);
	}
#endif  

	reg = readl(priv->control + 4);
	reg &= ~(A375_UNIT_CONTROL_MASK << A375_UNIT_CONTROL_SHIFT);
	reg &= ~A375_READOUT_INVERT;
	reg &= ~A375_HW_RESETn;

#if defined(MY_ABC_HERE)
	 
#else
	if (quirk_needed)
		reg |= A375_Z1_WORKAROUND_BIT;

#endif  
	writel(reg, priv->control + 4);
	mdelay(20);

	reg |= A375_HW_RESETn;
	writel(reg, priv->control + 4);
	mdelay(50);
}

static void armada380_init_sensor(struct platform_device *pdev,
				  struct armada_thermal_priv *priv)
{
#if defined(MY_ABC_HERE)
	unsigned long reg = readl_relaxed(priv->control + A380_CONTROL_MSB_OFFSET);
#else
	unsigned long reg = readl_relaxed(priv->control);
#endif  

	if (!(reg & A380_HW_RESET)) {
		reg |= A380_HW_RESET;
#if defined(MY_ABC_HERE)
		writel(reg, priv->control + A380_CONTROL_MSB_OFFSET);
#else
		writel(reg, priv->control);
#endif  
		mdelay(10);
	}

#if defined(MY_ABC_HERE)
	 
	reg = readl_relaxed(priv->control);
	reg &= ~A380_TSEN_TC_TRIM_MASK;
	reg |= 0x3;
	writel(reg, priv->control);

	armada380_temp_set_threshold(pdev, priv);

	reg = readl_relaxed(priv->dfx + 0x10);

	reg = readl_relaxed(priv->dfx + 0x14);
	reg |= (0x3 << 1);
	writel(reg, priv->dfx + 0x14);

	reg = readl_relaxed(priv->dfx + 0x4);
	reg |= (0x3 << 1);
	writel(reg, priv->dfx + 0x4);
#endif  
}
#endif  

static bool armada_is_valid(struct armada_thermal_priv *priv)
{
	unsigned long reg = readl_relaxed(priv->sensor);

#if defined(MY_ABC_HERE)
	return (reg >> priv->data->is_valid_shift) & THERMAL_VALID_MASK;
#else  
	return (reg >> THERMAL_VALID_OFFSET) & THERMAL_VALID_MASK;
#endif  
}

static int armada_get_temp(struct thermal_zone_device *thermal,
			  unsigned long *temp)
{
	struct armada_thermal_priv *priv = thermal->devdata;
	unsigned long reg;
#if defined(MY_ABC_HERE)
	unsigned long m, b, div;
#endif  

#if defined(MY_ABC_HERE)
	if (priv->data->is_valid && !priv->data->is_valid(priv)) {
#else  
	if (priv->ops->is_valid && !priv->ops->is_valid(priv)) {
#endif  
		dev_err(&thermal->device,
			"Temperature sensor reading not valid\n");
		return -EIO;
	}

	reg = readl_relaxed(priv->sensor);
#if defined(MY_ABC_HERE)
	reg = (reg >> priv->data->temp_shift) & priv->data->temp_mask;

	b = priv->data->coef_b;
	m = priv->data->coef_m;
	div = priv->data->coef_div;

	if (priv->data->inverted)
		*temp = ((m * reg) - b) / div;
	else
		*temp = (b - (m * reg)) / div;
#else  
	reg = (reg >> THERMAL_TEMP_OFFSET) & THERMAL_TEMP_MASK;
	*temp = (3153000000UL - (10000000UL*reg)) / 13825;
#endif  
	return 0;
}

static struct thermal_zone_device_ops ops = {
	.get_temp = armada_get_temp,
};

#if defined(MY_ABC_HERE)
static irqreturn_t a38x_temp_irq_handler(int irq, void *data)
{
	struct armada_thermal_priv *priv = (struct armada_thermal_priv *)data;
	struct device *dev = &priv->pdev->dev;
	u32 reg;
	 
	reg = readl_relaxed(priv->dfx + 0x14);
	reg &= ~(0x3 << 1);
	writel(reg, priv->dfx + 0x14);
	 
	reg = readl_relaxed(priv->dfx + 0x10);
	if (reg & (0x3 << 1))
		dev_warn(dev, "Overheat critical %s threshold temperature reached\n",
			 (reg & (1 << 1)) ? "high" : "low");
	 
	reg = readl_relaxed(priv->dfx + 0x14);
	reg |= (0x3 << 1);
	writel(reg, priv->dfx + 0x14);
	return IRQ_HANDLED;
}
#endif  

#if defined(MY_ABC_HERE)
static const struct armada_thermal_data armadaxp_data = {
	.init_sensor = armadaxp_init_sensor,
	.temp_shift = 10,
	.temp_mask = 0x1ff,
	.coef_b = 3153000000UL,
	.coef_m = 10000000UL,
	.coef_div = 13825,
};

static const struct armada_thermal_data armada370_data = {
	.is_valid = armada_is_valid,
	.init_sensor = armada370_init_sensor,
	.is_valid_shift = 9,
	.temp_shift = 10,
	.temp_mask = 0x1ff,
	.coef_b = 3153000000UL,
	.coef_m = 10000000UL,
	.coef_div = 13825,
};

static const struct armada_thermal_data armada375_data = {
	.is_valid = armada_is_valid,
	.init_sensor = armada375_init_sensor,
	.is_valid_shift = 10,
	.temp_shift = 0,
	.temp_mask = 0x1ff,
	.coef_b = 3171900000UL,
	.coef_m = 10000000UL,
	.coef_div = 13616,
};

static const struct armada_thermal_data armada380_data = {
	.is_valid = armada_is_valid,
	.init_sensor = armada380_init_sensor,
	.is_valid_shift = 10,
	.temp_shift = 0,
	.temp_mask = 0x3ff,
#if defined(MY_ABC_HERE)
	.coef_b = 2931108200UL,
	.coef_m = 5000000UL,
	.coef_div = 10502,
#else  
	.coef_b = 1169498786UL,
	.coef_m = 2000000UL,
	.coef_div = 4289,
#endif  
	.inverted = true,
};
#else  
static const struct armada_thermal_ops armadaxp_ops = {
	.init_sensor = armadaxp_init_sensor,
};

static const struct armada_thermal_ops armada370_ops = {
	.is_valid = armada_is_valid,
	.init_sensor = armada370_init_sensor,
};
#endif  

static const struct of_device_id armada_thermal_id_table[] = {
	{
		.compatible = "marvell,armadaxp-thermal",
#if defined(MY_ABC_HERE)
		.data       = &armadaxp_data,
#else  
		.data       = &armadaxp_ops,
#endif  
	},
	{
		.compatible = "marvell,armada370-thermal",
#if defined(MY_ABC_HERE)
		.data       = &armada370_data,
#else  
		.data       = &armada370_ops,
#endif  
	},
#if defined(MY_ABC_HERE)
	{
		.compatible = "marvell,armada375-thermal",
		.data       = &armada375_data,
	},
#if defined(MY_ABC_HERE)
	 
#else
	{
		.compatible = "marvell,armada375-z1-thermal",
		.data       = &armada375_data,
	},
#endif  
	{
		.compatible = "marvell,armada380-thermal",
		.data       = &armada380_data,
	},
#endif  
	{
		 
	},
};
MODULE_DEVICE_TABLE(of, armada_thermal_id_table);

static int armada_thermal_probe(struct platform_device *pdev)
{
	struct thermal_zone_device *thermal;
	const struct of_device_id *match;
	struct armada_thermal_priv *priv;
	struct resource *res;
#if defined(MY_ABC_HERE)
	int irq;
#endif  

	match = of_match_device(armada_thermal_id_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->sensor = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->sensor))
		return PTR_ERR(priv->sensor);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	priv->control = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->control))
		return PTR_ERR(priv->control);

#if defined(MY_ABC_HERE)
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	priv->dfx = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->dfx))
		return PTR_ERR(priv->dfx);
#endif  

#if defined(MY_ABC_HERE)
	priv->data = (struct armada_thermal_data *)match->data;
	priv->data->init_sensor(pdev, priv);
#else  
	priv->ops = (struct armada_thermal_ops *)match->data;
	priv->ops->init_sensor(priv);
#endif  

	thermal = thermal_zone_device_register("armada_thermal", 0, 0,
					       priv, &ops, NULL, 0, 0);
	if (IS_ERR(thermal)) {
		dev_err(&pdev->dev,
			"Failed to register thermal zone device\n");
		return PTR_ERR(thermal);
	}

#if defined(MY_ABC_HERE)
	priv->pdev = pdev;
	 
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_warn(&pdev->dev, "no irq\n");
		return irq;
	}
	if (devm_request_irq(&pdev->dev, irq, a38x_temp_irq_handler,
				0, pdev->name, priv) < 0) {
		dev_warn(&pdev->dev, "Interrupt not available.\n");
	}
#endif  

	platform_set_drvdata(pdev, thermal);

#if defined(MY_ABC_HERE)
	g_syno_tz = thermal;
#endif  

	return 0;
}

static int armada_thermal_exit(struct platform_device *pdev)
{
	struct thermal_zone_device *armada_thermal =
		platform_get_drvdata(pdev);

	thermal_zone_device_unregister(armada_thermal);
#if defined(MY_ABC_HERE)
	 
#else  
	platform_set_drvdata(pdev, NULL);
#endif  

	return 0;
}

#if defined(MY_ABC_HERE)
static int armada_thermal_resume(struct platform_device *pdev)
{
	struct thermal_zone_device *thermal =
		platform_get_drvdata(pdev);
	struct armada_thermal_priv *priv = thermal->devdata;

	priv->data->init_sensor(pdev, priv);

	return 0;
}
#endif  

static struct platform_driver armada_thermal_driver = {
	.probe = armada_thermal_probe,
	.remove = armada_thermal_exit,
#if defined(MY_ABC_HERE) && defined(CONFIG_PM)
	.resume = armada_thermal_resume,
#endif  
	.driver = {
		.name = "armada_thermal",
		.owner = THIS_MODULE,
#if defined(MY_ABC_HERE)
		.of_match_table = armada_thermal_id_table,
#else  
		.of_match_table = of_match_ptr(armada_thermal_id_table),
#endif  
	},
};

module_platform_driver(armada_thermal_driver);

MODULE_AUTHOR("Ezequiel Garcia <ezequiel.garcia@free-electrons.com>");
#if defined(MY_ABC_HERE)
MODULE_DESCRIPTION("Armada 370/380/XP thermal driver");
#else
MODULE_DESCRIPTION("Armada 370/XP thermal driver");
#endif  
MODULE_LICENSE("GPL v2");
