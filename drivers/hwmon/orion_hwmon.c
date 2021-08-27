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
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <plat/orion_hwmon.h>

#define THERMAL_SENS_STATUS_REG         0x10078

#define THERMAL_VALID_OFFS              9
#define THERMAL_VALID_MASK              0x1

#define THERMAL_SENS_OFFS               10
#define THERMAL_SENS_MASK               0x1FF

#define THERMAL_SENS_MAX_TIME_READ      16
#define KW_TEMPER_FORMULA(x)            (((322 - x) * 10000) / 13625) /* in Celsius */

typedef enum {
	SHOW_TEMP,
	SHOW_NAME
} SHOW;

struct orion_hwmon {
	struct device	*dev;
	void __iomem	*thermal_stat;
};

static int kw_temp_read_temp(struct device *dev)
{
	struct orion_hwmon *hwmon = dev_get_drvdata(dev);
	u32 reg = 0, reg1 = 0, i, temp_cels;

	/* Read the Thermal Sensor Status Register */
	reg = readl(hwmon->thermal_stat);

	/* Check if temperature reading is valid */
	if (((reg >> THERMAL_VALID_OFFS) & THERMAL_VALID_MASK) != 0x1) {
		/* No valid temperature */
		printk(KERN_ERR "kw-hwmon: The reading temperature is not valid !\n");
		return -EIO;
	}

	/*
	 * Read the thermal sensor looking for two
	 * successive readings that differ in LSb only.
	 */
	for (i = 0; i < THERMAL_SENS_MAX_TIME_READ; i++) {
		/* Read the raw temperature */
		reg = readl(hwmon->thermal_stat);
		reg >>= THERMAL_SENS_OFFS;
		reg &= THERMAL_SENS_MASK;

		if (((reg ^ reg1) & 0x1FE) == 0x0)
			break;
		 /* Save the current reading for the next iteration */
		reg1 = reg;
	}

	if (i == THERMAL_SENS_MAX_TIME_READ)
		printk(KERN_WARNING "kw-hwmon: Thermal sensor is unstable!\n");

	/* Convert the temperature to celsium */
	temp_cels = KW_TEMPER_FORMULA(reg);

	return temp_cels;
}

/*
 * Sysfs stuff
 */
static ssize_t show_name(struct device *dev, struct device_attribute
			 *devattr, char *buf)
{
	return sprintf(buf, "%s\n", "kw-hwmon");
}

static ssize_t show_temp(struct device *dev,
			 struct device_attribute *devattr, char *buf)
{
	return sprintf(buf, "%d\n", kw_temp_read_temp(dev));
}

static SENSOR_DEVICE_ATTR(temp_input, S_IRUGO, show_temp, NULL, SHOW_TEMP);
static SENSOR_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, SHOW_NAME);

static struct attribute *kw_temp_attributes[] = {
	&sensor_dev_attr_name.dev_attr.attr,
	&sensor_dev_attr_temp_input.dev_attr.attr,
	NULL
};

static const struct attribute_group kw_temp_group = {
	.attrs	= kw_temp_attributes,
};

static int __devinit kw_temp_probe(struct platform_device *pdev)
{
	struct orion_hwmon *hwmon;
	struct resource *res;
	struct device *dev;
	int err;

	hwmon = kzalloc(sizeof(*hwmon), GFP_KERNEL);
	if (!hwmon) {
		err = -ENOMEM;
		goto out;
	}

	err = sysfs_create_group(&pdev->dev.kobj, &kw_temp_group);
	if (err)
		goto err_sysfs;

	dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(dev)) {
		dev_err(&pdev->dev, "Class registration failed (%d)\n", err);
		goto out;
	}
	hwmon->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err = -ENODEV;
		goto out;
	}

	if (!request_mem_region(res->start, resource_size(res),
				dev_name(&pdev->dev))) {
		err = -EBUSY;
		goto out;
	}

	hwmon->thermal_stat = ioremap(res->start, resource_size(res));
	if (!hwmon->thermal_stat) {
		err = -ENOMEM;
		goto out;
	}

	platform_set_drvdata(pdev, hwmon);
	printk(KERN_INFO "Kirkwood hwmon thermal sensor initialized.\n");

	return 0;

out:
	sysfs_remove_group(&pdev->dev.kobj, &kw_temp_group);
err_sysfs:

	return err;
}

static int __devexit kw_temp_remove(struct platform_device *pdev)
{
	struct kw_temp_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(&pdev->dev);
	sysfs_remove_group(&pdev->dev.kobj, &kw_temp_group);
	platform_set_drvdata(pdev, NULL);
	kfree(data);

	return 0;
}

static struct platform_driver kw_temp_driver = {
	.probe		= kw_temp_probe,
	.remove		= __exit_p(kw_temp_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= ORION_HWMON_NAME,
	},
};

module_platform_driver(kw_temp_driver);

MODULE_AUTHOR("Marvell Semiconductors");
MODULE_DESCRIPTION("Marvell Kirkwood SoC hwmon (thermal sensor) driver");
MODULE_LICENSE("GPL");
