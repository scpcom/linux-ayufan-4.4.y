#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * hwmon-axp.c - temperature monitoring driver for Dove SoC
 *
 * Inspired from other hwmon drivers
 *
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
//#include <linux/delay.h>
#include <linux/slab.h>
//#include "pmu/mvPmuRegs.h"
 
/* Termal Sensor Registers */
#define TSEN_STATUS_REG				0x18300
#define	TSEN_STATUS_TEMP_OUT_OFFSET		19
#define	TSEN_STATUS_TEMP_OUT_MASK		(0x1FF << TSEN_STATUS_TEMP_OUT_OFFSET)

#define TSEN_CONF_REG				0x18304
#define	TSEN_CONF_OTF_CALIB_MASK		(0x1 << 30)
#define	TSEN_CONF_REF_CAL_MASK			(0x1FF << 11)
#define	TSEN_CONF_SOFT_RESET_MASK		(0x1 << 1)

#define ARMADAXP_OVERHEAT_TEMP	105		/* milidegree Celsius */
#define ARMADAXP_OVERHEAT_DELAY	0x700
#define ARMADAXP_OVERCOOL_TEMP	10		/* milidegree Celsius */
#define	ARMADAXP_OVERCOOL_DELAY	0x700
#define ARMADAXP_OVERHEAT_MIN	0
#define ARMADAXP_OVERHEAT_MAX	110
#define ARMADAXP_OVERCOOL_MIN	0
#define ARMADAXP_OVERCOOL_MAX	110

/* Junction Temperature */
#define ARMADAXP_TSEN_TEMP2RAW(x) ((3153000 - (13825 * x)) / 10000)
#define ARMADAXP_TSEN_RAW2TEMP(x) ((3153000 - (10000 * x)) / 13825)
#if 0
/* Dove */
((2281638 - (10 * x)) / 7298)    /* in millCelsius */
 ((2281638 - (7298 * x)) / 10)
#endif

#define LABEL "T-junction"
static struct device *hwmon_dev;
unsigned int temp_min = ARMADAXP_OVERCOOL_TEMP;
unsigned int temp_max = ARMADAXP_OVERHEAT_TEMP;

typedef enum {
	SHOW_TEMP,
	TEMP_MAX,
	TEMP_MIN,
	SHOW_NAME,
	SHOW_TYPE,
	SHOW_LABEL } SHOW;

static void axptemp_set_thresholds(unsigned int max, unsigned int min)
{
#if 0
	u32 temp, reg;

	/* Set the overheat threashold & delay */
	temp = ARMADAXP_TSEN_TEMP2RAW(max);
	reg = readl(INTER_REGS_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_OVRHEAT_THRSH_MASK;
	reg |= (temp << PMU_TM_OVRHEAT_THRSH_OFFS);
	writel(reg, (INTER_REGS_BASE | PMU_THERMAL_MNGR_REG));

	/* Set the overcool threshole & delay */
	temp = ARMADAXP_TSEN_TEMP2RAW(min);
	reg = readl(INTER_REGS_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_COOL_THRSH_MASK;
	reg |= (temp << PMU_TM_COOL_THRSH_OFFS);
	writel(reg, (INTER_REGS_BASE | PMU_THERMAL_MNGR_REG));
#endif
}

static int axptemp_init_sensor(void)
{
	u32 reg;

	/* init the TSEN sensor once */
	reg = readl(INTER_REGS_BASE | TSEN_CONF_REG);
	reg |= TSEN_CONF_OTF_CALIB_MASK;
	writel(reg, (INTER_REGS_BASE | TSEN_CONF_REG));

	reg = readl(INTER_REGS_BASE | TSEN_CONF_REG);
	reg &= ~(TSEN_CONF_REF_CAL_MASK);
	reg |= (0xf1 << 11);
	writel(reg, (INTER_REGS_BASE | TSEN_CONF_REG));

	reg = readl(INTER_REGS_BASE | TSEN_CONF_REG);
	reg |= TSEN_CONF_SOFT_RESET_MASK;
	writel(reg, (INTER_REGS_BASE | TSEN_CONF_REG));

	//udelay(1000);

	reg = readl(INTER_REGS_BASE | TSEN_CONF_REG);
	reg &= ~(TSEN_CONF_SOFT_RESET_MASK);
	writel(reg, (INTER_REGS_BASE | TSEN_CONF_REG));

	//udelay(10000);
	 
#if 0
	/* Set thresholds */
	axptemp_set_thresholds(temp_max, temp_min);

	/* Set delays */
	writel(ARMADAXP_OVERHEAT_DELAY, (INTER_REGS_BASE | PMU_TM_OVRHEAT_DLY_REG));
	writel(ARMADAXP_OVERCOOL_DELAY, (INTER_REGS_BASE | PMU_TM_COOLING_DLY_REG));
#endif

	return 0;
}

#ifdef CONFIG_SYNO_ARMADA_ARCH
static int axptemp_read_temp(void)
#else
int axptemp_read_temp(void)
#endif
{
	int reg;

	reg = readl(INTER_REGS_BASE | TSEN_STATUS_REG);
	reg = (reg & TSEN_STATUS_TEMP_OUT_MASK) >> TSEN_STATUS_TEMP_OUT_OFFSET;
//	value = ((3153000 - (10000 * reg)) / 13825);

	return ARMADAXP_TSEN_RAW2TEMP(reg);
}
#ifdef CONFIG_SYNO_ARMADA_ARCH
EXPORT_SYMBOL(axptemp_read_temp);
#endif

/*
 * Sysfs stuff
 */

static ssize_t show_name(struct device *dev, struct device_attribute
			  *devattr, char *buf) {
	return sprintf(buf, "%s\n", "axp-hwmon");
}

static ssize_t show_alarm(struct device *dev, struct device_attribute
			  *devattr, char *buf)
{
#if 0
	int alarm = 0;
	u32 reg;

	reg = readl(INTER_REGS_BASE | PMU_INT_CAUSE_REG);
	if (reg & PMU_INT_OVRHEAT_MASK)
	{
		alarm = 1;
		writel ((reg & ~PMU_INT_OVRHEAT_MASK), (INTER_REGS_BASE | PMU_INT_CAUSE_REG));
	}
	else if (reg & PMU_INT_COOLING_MASK)
	{
		alarm = 2;
		writel ((reg & ~PMU_INT_COOLING_MASK), (INTER_REGS_BASE | PMU_INT_CAUSE_REG));
	}
#endif
	return sprintf(buf, "%d\n", 0);
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
		ret = sprintf(buf, "%d\n", axptemp_read_temp());
	else if (attr->index == TEMP_MAX)
		ret = sprintf(buf, "%d\n", temp_max);
	else if (attr->index == TEMP_MIN)
		ret = sprintf(buf, "%d\n", temp_min);
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
		if((temp < ARMADAXP_OVERHEAT_MIN) || (temp > ARMADAXP_OVERHEAT_MAX))
			printk(KERN_WARNING "Invalid max temperature input (out of range: %d-%d)!",
				ARMADAXP_OVERHEAT_MIN, ARMADAXP_OVERHEAT_MAX);
		else {
			temp_max = temp;
			axptemp_set_thresholds(temp_max, temp_min);
		}
	}
	else if (attr->index == TEMP_MIN) {
		if((temp < ARMADAXP_OVERCOOL_MIN) || (temp > ARMADAXP_OVERCOOL_MAX))
			printk(KERN_WARNING "Invalid min temperature input (out of range: %d-%d)!",
				ARMADAXP_OVERCOOL_MIN, ARMADAXP_OVERCOOL_MAX);
		else {
			temp_min = temp;
			axptemp_set_thresholds(temp_max, temp_min);
		}
	}
	else
		printk(KERN_ERR "axp-temp: Invalid sensor attribute!");

	printk(KERN_INFO "set_temp got string: %d\n", temp);

	return count;
}

/* TODO - Add read/write support in order to support setting max/min */
static SENSOR_DEVICE_ATTR(temp1_type, S_IRUGO, show_info, NULL,
			  SHOW_TYPE);
static SENSOR_DEVICE_ATTR(temp1_label, S_IRUGO, show_info, NULL,
			  SHOW_LABEL);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp, NULL,
			  SHOW_TEMP);
static SENSOR_DEVICE_ATTR(temp1_max, S_IRWXUGO, show_temp, set_temp,
			  TEMP_MAX);
static SENSOR_DEVICE_ATTR(temp1_min, S_IRWXUGO, show_temp, set_temp,
			  TEMP_MIN);
static DEVICE_ATTR(temp1_crit_alarm, S_IRUGO, show_alarm, NULL);
static SENSOR_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, SHOW_NAME);

static struct attribute *axptemp_attributes[] = {
	&sensor_dev_attr_name.dev_attr.attr,
	&dev_attr_temp1_crit_alarm.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp1_min.dev_attr.attr,
	&sensor_dev_attr_temp1_type.dev_attr.attr,
	&sensor_dev_attr_temp1_label.dev_attr.attr,
	NULL
};

static const struct attribute_group axptemp_group = {
	.attrs = axptemp_attributes,
};

static int __devinit axptemp_probe(struct platform_device *pdev)
{
	int err;

	err = axptemp_init_sensor();
	if (err)
		goto exit;

	err = sysfs_create_group(&pdev->dev.kobj, &axptemp_group);
	if (err)
		goto exit;

	hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "Class registration failed (%d)\n",
			err);
		goto exit;
	}

	printk(KERN_INFO "Armada XP hwmon thermal sensor initialized.\n");

	return 0;

exit:
	sysfs_remove_group(&pdev->dev.kobj, &axptemp_group);
	return err;
}

static int __devexit axptemp_remove(struct platform_device *pdev)
{
	struct axptemp_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &axptemp_group);
	platform_set_drvdata(pdev, NULL);
	kfree(data);
	return 0;
}

static int axptemp_resume(struct platform_device *dev)
{
	return axptemp_init_sensor();
}

static struct platform_driver axptemp_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "axp-temp",
	},
	.probe = axptemp_probe,
	.remove = __devexit_p(axptemp_remove),
	.resume = axptemp_resume,
};

static int __init axptemp_init(void)
{
	return platform_driver_register(&axptemp_driver);
}

static void __exit axptemp_exit(void)
{
	platform_driver_unregister(&axptemp_driver);
}

MODULE_AUTHOR("Marvell Semiconductors");
MODULE_DESCRIPTION("Marvell Armada XP SoC hwmon driver");
MODULE_LICENSE("GPL");

module_init(axptemp_init)
module_exit(axptemp_exit)
