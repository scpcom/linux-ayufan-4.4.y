 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#define TSEN_STATUS_REG				0xE8078
#define	TSEN_STATUS_TEMP_OUT_MASK		0x1FF
#define	TSEN_STATUS_TEMP_VALID_MASK		0x400

#define TSEN_CONT_MSB_REG			0xE8080
#define TSEN_CONT_MSB_UNIT_CTRL_OFFSET		27
#define TSEN_CONT_MSB_UNIT_CTRL_MASK		(0x7 << TSEN_CONT_MSB_UNIT_CTRL_OFFSET)
#define TSEN_CONT_MSB_READOUT_INVERT_OFFSET	15
#define TSEN_CONT_MSB_READOUT_INVERT_MASK	(0x1 << TSEN_CONT_MSB_READOUT_INVERT_OFFSET)
#define TSEN_CONT_MSB_DISABLE_RST_OFFSET	8
#define TSEN_CONT_MSB_DISABLE_RST_MASK		(0x1 << TSEN_CONT_MSB_DISABLE_RST_OFFSET)

#define A375_OVERHEAT_TEMP	105		 
#define A375_OVERHEAT_DELAY	0x700
#define A375_OVERHEAT_MIN	0
#define A375_OVERHEAT_MAX	110000

#define PMU_THERMAL_MNGR_REG	0x184c4
#define	PMU_INT_MASK_REG        0x1C124
#define	PMU_INT_CAUSE_REG	0x1c120
#define PMU_INT_OVRHEAT_MASK	0x1
#define PMU_INT_COOLING_MASK	0x2

#define PMU_TM_COOL_THRSH_OFFS          10
#define PMU_TM_COOL_THRSH_MASK          (0x1FF << PMU_TM_COOL_THRSH_OFFS)
#define PMU_TM_OVRHEAT_THRSH_OFFS       19
#define PMU_TM_OVRHEAT_THRSH_MASK       (0x1FF << PMU_TM_OVRHEAT_THRSH_OFFS)

#define PMU_TM_DISABLE_OFFS             0
#define PMU_TM_DISABLE_MASK             (0x1 << PMU_TM_DISABLE_OFFS)

#define	PMU_TM_OVRHEAT_DLY_REG  0x184cc
#define	PMU_TM_COOLING_DLY_REG	0x184c8

#define A375_TSEN_TEMP2RAW(x) ((3239600 - (13616 * x)) / 10000)
#define A375_TSEN_RAW2TEMP(x) ((3239600 - (10000 * x)) / 13616)

#define LABEL "T-junction"
static struct device *hwmon_dev;
unsigned int temp_max = A375_OVERHEAT_TEMP;

typedef enum {
	SHOW_TEMP,
	TEMP_MAX,
	SHOW_NAME,
	SHOW_TYPE,
	SHOW_LABEL } SHOW;

#define	SYNO_A375_TEMP
#ifdef SYNO_A375_TEMP
#define IRQ_AURORA_PMU           107  
#endif

static void a375_temp_set_thresholds(unsigned int max)
{
	u32 temp, reg;

	reg = readl(INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_DISABLE_MASK;
	writel(reg, (INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG));

	temp = A375_TSEN_TEMP2RAW(max);
	reg = readl(INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_OVRHEAT_THRSH_MASK;
	reg |= (temp << PMU_TM_OVRHEAT_THRSH_OFFS);
	writel(reg, (INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG));
}

static int a375_temp_init_sensor(void)
{
	u32 reg;

	reg = readl(INTER_REGS_VIRT_BASE | TSEN_CONT_MSB_REG);
	reg &= ~TSEN_CONT_MSB_UNIT_CTRL_MASK;
	reg |= (0x0 << TSEN_CONT_MSB_UNIT_CTRL_OFFSET);

	reg &= ~TSEN_CONT_MSB_READOUT_INVERT_MASK;
	reg |= (0x0 << TSEN_CONT_MSB_READOUT_INVERT_OFFSET);

	reg &= ~TSEN_CONT_MSB_DISABLE_RST_MASK;
	reg |= (0x0 << TSEN_CONT_MSB_DISABLE_RST_OFFSET);
	writel(reg, (INTER_REGS_VIRT_BASE | TSEN_CONT_MSB_REG));

	udelay(20);

	reg &= ~TSEN_CONT_MSB_DISABLE_RST_MASK;
	reg |= (0x1 << TSEN_CONT_MSB_DISABLE_RST_OFFSET);
	writel(reg, (INTER_REGS_VIRT_BASE | TSEN_CONT_MSB_REG));

	return 0;
}

static int a375_temp_read_temp(void)
{
	int reg;
	int timeOut = 0;

	do {
		reg = readl(INTER_REGS_VIRT_BASE | TSEN_STATUS_REG);
		udelay(20);
		timeOut++;
		if (timeOut > 1000)
			return 0;

	} while (!(reg & TSEN_STATUS_TEMP_VALID_MASK));

	reg = reg & TSEN_STATUS_TEMP_OUT_MASK;
	
	return A375_TSEN_RAW2TEMP(reg);
}

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
extern unsigned int mvCtrlGetJuncTemp(void);
int axptemp_read_temp(void)
{
	#if 0  
	int val = a375_temp_read_temp();
	printk("CPU Temp: %d\n", val);
	return val;
	#else
	return mvCtrlGetJuncTemp();
	#endif
}
EXPORT_SYMBOL(axptemp_read_temp);
#endif

static ssize_t show_name(struct device *dev, struct device_attribute
			  *devattr, char *buf) {
	return sprintf(buf, "%s\n", "a375-hwmon");
}

static ssize_t show_alarm(struct device *dev, struct device_attribute
			  *devattr, char *buf)
{
	int alarm = 0;
	u32 reg;

	reg = readl(INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG);
	if (reg & PMU_INT_OVRHEAT_MASK) {
		alarm = 1;
		writel((reg & ~PMU_INT_OVRHEAT_MASK), (INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG));
	}

	return sprintf(buf, "%d\n", alarm);
}

static ssize_t show_info(struct device *dev,
			 struct device_attribute *devattr, char *buf) {
	int ret;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

	if (attr->index == SHOW_TYPE)
		ret = sprintf(buf, "%d\n", 3);
	else if (attr->index == SHOW_LABEL)
		ret = sprintf(buf, "%s\n", LABEL);
	else
		ret = sprintf(buf, "%d\n", -1);
	return ret;
}

static ssize_t show_temp(struct device *dev,
			 struct device_attribute *devattr, char *buf) {
	int ret;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

	if (attr->index == SHOW_TEMP)
		ret = sprintf(buf, "%d\n", a375_temp_read_temp());
	else if (attr->index == TEMP_MAX)
		ret = sprintf(buf, "%d\n", temp_max);
	else
		ret = sprintf(buf, "%d\n", -1);

	return ret;
}

static ssize_t set_temp(struct device *dev, struct device_attribute *devattr,
			 const char *buf, size_t count) {

	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	unsigned int temp;

	if (sscanf(buf, "%d", &temp) != 1)
		printk(KERN_WARNING "Invalid input string for temperature!");

	if (attr->index == TEMP_MAX) {
		if (temp > A375_OVERHEAT_MAX)
			printk(KERN_WARNING "Invalid max temperature input (out of range: %d-%d)!",
				A375_OVERHEAT_MIN, A375_OVERHEAT_MAX);
		else {
			temp_max = temp;
			a375_temp_set_thresholds(temp_max);
		}
	} else
		printk(KERN_ERR "a375-temp: Invalid sensor attribute!");

	writel(0, (INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG));
	writel(PMU_INT_OVRHEAT_MASK, (INTER_REGS_VIRT_BASE | PMU_INT_MASK_REG));

	printk(KERN_INFO "set_temp got string: %d\n", temp);
	return count;
}

static irqreturn_t a375_temp_irq_handler(int irq, void *data)
{
	u32 val, mask;
	mask = readl(INTER_REGS_VIRT_BASE | PMU_INT_MASK_REG);
	val = (readl(INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG) & mask);
	 
	writel((mask & ~val), (INTER_REGS_VIRT_BASE | PMU_INT_MASK_REG));

	printk(KERN_WARNING "WARNING: %s threshold was triggered\n",
			((val & PMU_INT_OVRHEAT_MASK) ? "overheat" : "cooling"));

	if (val & PMU_INT_OVRHEAT_MASK)
		val &= ~PMU_INT_OVRHEAT_MASK;

	writel(val, (INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG));

	return IRQ_HANDLED;
}

static SENSOR_DEVICE_ATTR(temp1_type, S_IRUGO, show_info, NULL,
			  SHOW_TYPE);
static SENSOR_DEVICE_ATTR(temp1_label, S_IRUGO, show_info, NULL,
			  SHOW_LABEL);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp, NULL,
			  SHOW_TEMP);
static SENSOR_DEVICE_ATTR(temp1_max, S_IRWXUGO, show_temp, set_temp,
			  TEMP_MAX);
static DEVICE_ATTR(temp1_crit_alarm, S_IRUGO, show_alarm, NULL);
static SENSOR_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, SHOW_NAME);

static struct attribute *a375_temp_attributes[] = {
	&sensor_dev_attr_name.dev_attr.attr,
	&dev_attr_temp1_crit_alarm.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp1_type.dev_attr.attr,
	&sensor_dev_attr_temp1_label.dev_attr.attr,
	NULL
};

static const struct attribute_group a375_temp_group = {
	.attrs = a375_temp_attributes,
};

static int __devinit a375_temp_probe(struct platform_device *pdev)
{
	int err;

	err = a375_temp_init_sensor();
	if (err)
		goto exit;

	err = sysfs_create_group(&pdev->dev.kobj, &a375_temp_group);
	if (err)
		goto exit;

	hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "Class registration failed (%d)\n",
			err);
		goto exit;
	}
	printk(KERN_INFO "Armada 375 hwmon thermal sensor initialized.\n");

	return 0;

exit:
	sysfs_remove_group(&pdev->dev.kobj, &a375_temp_group);
	return err;
}

static int __devexit a375_temp_remove(struct platform_device *pdev)
{
	struct a375_temp_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &a375_temp_group);
	platform_set_drvdata(pdev, NULL);
	kfree(data);
	return 0;
}

static int a375_temp_resume(struct platform_device *dev)
{
	return a375_temp_init_sensor();
}

static struct platform_driver a375_temp_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "a375-temp",
	},
	.probe = a375_temp_probe,
	.remove = __devexit_p(a375_temp_remove),
	.resume = a375_temp_resume,
};

static int __init a375_temp_init(void)
{
	return platform_driver_register(&a375_temp_driver);
}

static void __exit a375_temp_exit(void)
{
	platform_driver_unregister(&a375_temp_driver);
}

MODULE_AUTHOR("Marvell Semiconductors");
MODULE_DESCRIPTION("Marvell Armada 375 SoC hwmon driver");
MODULE_LICENSE("GPL");

module_init(a375_temp_init)
module_exit(a375_temp_exit)
