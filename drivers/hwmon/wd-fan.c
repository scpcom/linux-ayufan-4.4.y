#include <linux/kernel.h>
#include <linux/platform_device.h>    /* needed for 'resource' */
#include <linux/module.h>
#include <asm/io.h>                   /* needed for 'ioremap'  */
#include <linux/gpio-fan.h>
#include <linux/err.h>
#include <linux/rwsem.h>
#include <mach/gpio.h>
#include <linux/delay.h>

#define MIN_USABLE_PWM_VAL 7143

void __iomem *fan_port   = NULL;
static struct class *fans_class;
DECLARE_RWSEM(fans_list_lock);
LIST_HEAD(fans_list);

struct fan_classdev {
	const char    *name;
	unsigned int  percent_speed;
	struct device		*dev;
	struct list_head	 node;
};

static struct fan_classdev system_fan_dev = {
	.name                = "system_fan",
};


int fan_classdev_register(struct device *parent, struct fan_classdev *fan_cdev)
{
	fan_cdev->dev = device_create(fans_class, parent, 0, fan_cdev,"%s", fan_cdev->name);
	if (IS_ERR(fan_cdev->dev))
		return PTR_ERR(fan_cdev->dev);

	/* add to the list of leds */
	down_write(&fans_list_lock);
	list_add_tail(&fan_cdev->node, &fans_list);
	up_write(&fans_list_lock);

	printk(KERN_DEBUG "Registered fan device: %s\n", fan_cdev->name);

	return 0;
}


static ssize_t get_fan_percent_speed(struct device *dev, struct device_attribute *attr,
                                     const char *buf, size_t size) {

	struct fan_classdev *fan_cdev = dev_get_drvdata(dev);	
	
	if (!fan_cdev->percent_speed)
		fan_cdev->percent_speed = 0;
	
	return sprintf(buf, "%d\n", fan_cdev->percent_speed);	

}


static ssize_t set_fan_percent_speed(struct device *dev, struct device_attribute *attr,
                                     const char *buf, size_t size) {

	struct fan_classdev *fan_cdev = dev_get_drvdata(dev);
	unsigned int pwm_val = 7936;    /* Zero fan speed */
	char *after;
	unsigned long p_speed = simple_strtoul(buf, &after, 10);

	/* 
	 * Any value bigger than 100 is invalid. Also it was observed that
	 * values below 10 percent is not stable so values below 10 percent
	 * are not mapped. Hence starting value is 0d7143 (0x1be7). This is
	 * based on 0x0 (0d0) being fully ON and 0x1f00 (0d7936) being 
	 * the fully OFF state of the fan. This formula was calculated using 
	 * a linear interpolation
	 */
	 
	if (p_speed > 100)
		return -EINVAL;

	fan_cdev->percent_speed = p_speed;


	if (p_speed !=0)
		pwm_val = ((MIN_USABLE_PWM_VAL  * 100)-(MIN_USABLE_PWM_VAL*p_speed))/100;

	__raw_writel(0, COMCERTO_LOW_DUTY_PWM0);	
	msleep(100);
	__raw_writel(pwm_val, COMCERTO_LOW_DUTY_PWM0);


	return (ssize_t)size;
}



static struct device_attribute fan_class_attrs[] = {
	__ATTR(percent_speed, 0644, get_fan_percent_speed, set_fan_percent_speed),
	__ATTR_NULL,
};

static int __init sequoia_fan_init(struct platform_device *pdev) {
	int retval;
	struct gpio_fan_data *fan_data;
	fan_port = (void __iomem *) COMCERTO_GPIO_OUTPUT_REG;


	fans_class = class_create(THIS_MODULE, "fans");
	if (IS_ERR(fans_class))
		return PTR_ERR(fans_class);

	fans_class->dev_attrs = fan_class_attrs;

	retval = fan_classdev_register(NULL, &system_fan_dev);
	if (retval) {
		iounmap(fan_port);
		fan_port = NULL;
		return -1;
	}

	__raw_writel(0x80001f00, COMCERTO_MAX_EN_PWM0);
	__raw_writel(0x00001f00, COMCERTO_LOW_DUTY_PWM0);


	return 0;
}



module_init(sequoia_fan_init);

MODULE_AUTHOR("Arya Ahmadi-Ardakani <arya.ahmadi-ardakani@wdc.com>");
MODULE_LICENSE("GPL");
