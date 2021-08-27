/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

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

/* Termal Sensor Registers */
#define TSEN_STATUS_REG				0x184C4
#define	TSEN_STATUS_TEMP_OUT_OFFSET		1
#define	TSEN_STATUS_TEMP_OUT_MASK		(0x1FF << TSEN_STATUS_TEMP_OUT_OFFSET)

#define TSEN_CONF_REG				0x184D0
#define	TSEN_CONF_OTF_CALIB_MASK		(0x1 << 30)
#define	TSEN_CONF_START_CALIB_MASK		(0x1 << 25)
#define	TSEN_CONF_REF_CAL_MASK			(0x1FF << 11)
#define	TSEN_CONF_SOFT_RESET_MASK		(0x1 << 1)

#define A38X_OVERHEAT_TEMP	105	/* milidegree Celsius */
#define A38X_OVERHEAT_DELAY	0x700
#define A38X_OVERCOOL_TEMP	10	/* milidegree Celsius */
#define	A38X_OVERCOOL_DELAY	0x700
#define A38X_OVERHEAT_MIN	0
#define A38X_OVERHEAT_MAX	110000
#define A38X_OVERCOOL_MIN	0
#define A38X_OVERCOOL_MAX	110000
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

/* Junction Temperature */
#define A38X_TSEN_TEMP2RAW(x) ((3153000 - (13825 * x)) / 10000)
#define A38X_TSEN_RAW2TEMP(x) ((3153000 - (10000 * x)) / 13825)

#define LABEL "T-junction"
static struct device *hwmon_dev;
unsigned int temp_min = A38X_OVERCOOL_TEMP;
unsigned int temp_max = A38X_OVERHEAT_TEMP;

typedef enum {
	SHOW_TEMP,
	TEMP_MAX,
	TEMP_MIN,
	SHOW_NAME,
	SHOW_TYPE,
	SHOW_LABEL
} SHOW;

static void a38x_temp_set_thresholds(unsigned int max, unsigned int min)
{
	u32 temp, reg;

	reg = readl(INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_DISABLE_MASK;
	writel(reg, (INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG));

	/* Set the overheat threashold & delay */
	temp = A38X_TSEN_TEMP2RAW(max);
	reg = readl(INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_OVRHEAT_THRSH_MASK;
	reg |= (temp << PMU_TM_OVRHEAT_THRSH_OFFS);
	writel(reg, (INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG));

	/* Set the cool threshole & delay */
	temp = A38X_TSEN_TEMP2RAW(min);
	reg = readl(INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_COOL_THRSH_MASK;
	reg |= (temp << PMU_TM_COOL_THRSH_OFFS);
	writel(reg, (INTER_REGS_VIRT_BASE | PMU_THERMAL_MNGR_REG));
}

static int a38x_temp_init_sensor(void)
{
	u32 reg;

	/* init the TSEN sensor once */
	/* Enable On-The-Fly Calibration mode */
	reg = readl(INTER_REGS_VIRT_BASE | TSEN_CONF_REG);
	reg |= TSEN_CONF_OTF_CALIB_MASK;
	writel(reg, (INTER_REGS_VIRT_BASE | TSEN_CONF_REG));

	/* Set the Reference Count value */
	reg = readl(INTER_REGS_VIRT_BASE | TSEN_CONF_REG);
	reg &= ~(TSEN_CONF_REF_CAL_MASK);
	reg |= (0xf1 << 11);
	writel(reg, (INTER_REGS_VIRT_BASE | TSEN_CONF_REG));

	/* Do not start calibration sequence */
	reg = readl(INTER_REGS_VIRT_BASE | TSEN_CONF_REG);
	reg &= ~(TSEN_CONF_START_CALIB_MASK);
	writel(reg, (INTER_REGS_VIRT_BASE | TSEN_CONF_REG));

	/* Initiate Soft Reset
	   reg = readl(INTER_REGS_VIRT_BASE | TSEN_CONF_REG);
	   reg |= TSEN_CONF_SOFT_RESET_MASK;
	   writel(reg, (INTER_REGS_VIRT_BASE | TSEN_CONF_REG));
	 */
	/* udelay(1000); */

	/* Exit from Soft Reset
	   reg = readl(INTER_REGS_VIRT_BASE | TSEN_CONF_REG);
	   reg &= ~(TSEN_CONF_SOFT_RESET_MASK);
	   writel(reg, (INTER_REGS_VIRT_BASE | TSEN_CONF_REG));
	 */
	/* udelay(10000); */

	/* Set thresholds */
	a38x_temp_set_thresholds(temp_max, temp_min);

	/* Set delays */
	writel(A38X_OVERHEAT_DELAY,
	       (INTER_REGS_VIRT_BASE | PMU_TM_OVRHEAT_DLY_REG));
	writel(A38X_OVERCOOL_DELAY,
	       (INTER_REGS_VIRT_BASE | PMU_TM_COOLING_DLY_REG));

	/* Clear & unmask cooling/overheat interrupts */
	writel(0, (INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG));
	writel((PMU_INT_OVRHEAT_MASK | PMU_INT_COOLING_MASK),
	       (INTER_REGS_VIRT_BASE | PMU_INT_MASK_REG));

	return 0;
}

static int a38x_temp_read_temp(void)
{
	int reg;

	reg = readl(INTER_REGS_VIRT_BASE | TSEN_STATUS_REG);
	reg = (reg & TSEN_STATUS_TEMP_OUT_MASK) >> TSEN_STATUS_TEMP_OUT_OFFSET;
	return A38X_TSEN_RAW2TEMP(reg);
}

/*
 * Sysfs stuff
 */

static ssize_t show_name(struct device *dev, struct device_attribute
			 *devattr, char *buf)
{
	return sprintf(buf, "%s\n", "a38x-hwmon");
}

static ssize_t show_alarm(struct device *dev, struct device_attribute
			  *devattr, char *buf)
{
	int alarm = 0;
	u32 reg;

	reg = readl(INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG);
	if (reg & PMU_INT_OVRHEAT_MASK) {
		alarm = 1;
		writel((reg & ~PMU_INT_OVRHEAT_MASK),
		       (INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG));
	} else if (reg & PMU_INT_COOLING_MASK) {
		alarm = 2;
		writel((reg & ~PMU_INT_COOLING_MASK),
		       (INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG));
	}

	return sprintf(buf, "%d\n", alarm);
}

static ssize_t show_info(struct device *dev,
			 struct device_attribute *devattr, char *buf)
{
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
			 struct device_attribute *devattr, char *buf)
{
	int ret;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

	if (attr->index == SHOW_TEMP)
		ret = sprintf(buf, "%d\n", a38x_temp_read_temp());
	else if (attr->index == TEMP_MAX)
		ret = sprintf(buf, "%d\n", temp_max);
	else if (attr->index == TEMP_MIN)
		ret = sprintf(buf, "%d\n", temp_min);
	else
		ret = sprintf(buf, "%d\n", -1);

	return ret;
}

static ssize_t set_temp(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{

	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	unsigned int temp;

	if (sscanf(buf, "%d", &temp) != 1)
		printk(KERN_WARNING "Invalid input string for temperature!");

	if (attr->index == TEMP_MAX) {
		if ((temp < A38X_OVERHEAT_MIN) || (temp > A38X_OVERHEAT_MAX))
			printk(KERN_WARNING
			       "Invalid max temperature input (out of range: %d-%d)!",
			       A38X_OVERHEAT_MIN, A38X_OVERHEAT_MAX);
		else {
			temp_max = temp;
			a38x_temp_set_thresholds(temp_max, temp_min);
		}
	} else if (attr->index == TEMP_MIN) {
		if ((temp < A38X_OVERCOOL_MIN) || (temp > A38X_OVERCOOL_MAX))
			printk(KERN_WARNING
			       "Invalid min temperature input (out of range: %d-%d)!",
			       A38X_OVERCOOL_MIN, A38X_OVERCOOL_MAX);
		else {
			temp_min = temp;
			a38x_temp_set_thresholds(temp_max, temp_min);
		}
	} else
		printk(KERN_ERR "a38x-temp: Invalid sensor attribute!");

	/* Clear & unmask cooling/overheat interrupts */
	writel(0, (INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG));
	writel((PMU_INT_OVRHEAT_MASK | PMU_INT_COOLING_MASK),
	       (INTER_REGS_VIRT_BASE | PMU_INT_MASK_REG));

	printk(KERN_INFO "set_temp got string: %d\n", temp);
	return count;
}

static irqreturn_t a38x_temp_irq_handler(int irq, void *data)
{
	u32 val, mask;
	mask = readl(INTER_REGS_VIRT_BASE | PMU_INT_MASK_REG);
	val = (readl(INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG) & mask);
	/* Mask cooling/overheat interrupt */
	writel((mask & ~val), (INTER_REGS_VIRT_BASE | PMU_INT_MASK_REG));

	printk(KERN_WARNING "WARNING: %s threshold was triggered\n",
	       ((val & PMU_INT_OVRHEAT_MASK) ? "overheat" : "cooling"));

	if (val & PMU_INT_OVRHEAT_MASK)
		val &= ~PMU_INT_OVRHEAT_MASK;
	else if (val & PMU_INT_COOLING_MASK)
		val &= ~PMU_INT_COOLING_MASK;

	/* Clear cooling/overheat interrupt */
	writel(val, (INTER_REGS_VIRT_BASE | PMU_INT_CAUSE_REG));

	return IRQ_HANDLED;
}

/* TODO - Add read/write support in order to support setting max/min */
static SENSOR_DEVICE_ATTR(temp1_type, S_IRUGO, show_info, NULL, SHOW_TYPE);
static SENSOR_DEVICE_ATTR(temp1_label, S_IRUGO, show_info, NULL, SHOW_LABEL);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp, NULL, SHOW_TEMP);
static SENSOR_DEVICE_ATTR(temp1_max, S_IRWXUGO, show_temp, set_temp, TEMP_MAX);
static SENSOR_DEVICE_ATTR(temp1_min, S_IRWXUGO, show_temp, set_temp, TEMP_MIN);
static DEVICE_ATTR(temp1_crit_alarm, S_IRUGO, show_alarm, NULL);
static SENSOR_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, SHOW_NAME);

static struct attribute *a38x_temp_attributes[] = {
	&sensor_dev_attr_name.dev_attr.attr,
	&dev_attr_temp1_crit_alarm.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp1_min.dev_attr.attr,
	&sensor_dev_attr_temp1_type.dev_attr.attr,
	&sensor_dev_attr_temp1_label.dev_attr.attr,
	NULL
};

static const struct attribute_group a38x_temp_group = {
	.attrs = a38x_temp_attributes,
};

static int __devinit a38x_temp_probe(struct platform_device *pdev)
{
	int err, irq;

	err = a38x_temp_init_sensor();
	if (err)
		goto exit;

	err = sysfs_create_group(&pdev->dev.kobj, &a38x_temp_group);
	if (err)
		goto exit;

	hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "Class registration failed (%d)\n", err);
		goto exit;
	}

	/* Register cooling/overheat interrupt */
	irq = IRQ_AURORA_PMU;
	err = request_irq(irq, a38x_temp_irq_handler, IRQF_DISABLED,
			  "a38x-temp", NULL);
	if (err)
		printk(KERN_INFO "unable to request IRQ%d for a38x-temp\n",
		       irq);
	printk(KERN_INFO "Armada 38x hwmon thermal sensor initialized.\n");

	return 0;
exit:
	sysfs_remove_group(&pdev->dev.kobj, &a38x_temp_group);
	return err;
}

static int __devexit a38x_temp_remove(struct platform_device *pdev)
{
	struct a38x_temp_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &a38x_temp_group);
	platform_set_drvdata(pdev, NULL);
	kfree(data);
	return 0;
}

static int a38x_temp_resume(struct platform_device *dev)
{
	return a38x_temp_init_sensor();
}

static struct platform_driver a38x_temp_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "a38x-temp",
		   },
	.probe = a38x_temp_probe,
	.remove = __devexit_p(a38x_temp_remove),
	.resume = a38x_temp_resume,
};

static int __init a38x_temp_init(void)
{
	return platform_driver_register(&a38x_temp_driver);
}

static void __exit a38x_temp_exit(void)
{
	platform_driver_unregister(&a38x_temp_driver);
}

MODULE_AUTHOR("Marvell Semiconductors");
MODULE_DESCRIPTION("Marvell Armada 38x LP SoC hwmon driver");
MODULE_LICENSE("GPL");

module_init(a38x_temp_init)
module_exit(a38x_temp_exit)
