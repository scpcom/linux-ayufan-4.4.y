#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/idr.h>

#ifdef	DEBUG
#define	extra_checks	1
#else
#define	extra_checks	0
#endif

static DEFINE_SPINLOCK(gpio_lock);

struct gpio_desc {
	struct gpio_chip	*chip;
	unsigned long		flags;
 
#define FLAG_REQUESTED	0
#define FLAG_IS_OUT	1
#define FLAG_RESERVED	2
#define FLAG_EXPORT	3	 
#define FLAG_SYSFS	4	 
#define FLAG_TRIG_FALL	5	 
#define FLAG_TRIG_RISE	6	 

#define PDESC_ID_SHIFT	16	 

#define GPIO_FLAGS_MASK		((1 << PDESC_ID_SHIFT) - 1)
#define GPIO_TRIGGER_MASK	(BIT(FLAG_TRIG_FALL) | BIT(FLAG_TRIG_RISE))

#ifdef CONFIG_DEBUG_FS
	const char		*label;
#endif
};
static struct gpio_desc gpio_desc[ARCH_NR_GPIOS];

#ifdef CONFIG_GPIO_SYSFS
struct poll_desc {
	struct work_struct	work;
	struct sysfs_dirent	*value_sd;
};

static struct idr pdesc_idr;
#endif

static inline void desc_set_label(struct gpio_desc *d, const char *label)
{
#ifdef CONFIG_DEBUG_FS
	d->label = label;
#endif
}

static int gpio_ensure_requested(struct gpio_desc *desc, unsigned offset)
{
	const struct gpio_chip *chip = desc->chip;
	const int gpio = chip->base + offset;

	if (WARN(test_and_set_bit(FLAG_REQUESTED, &desc->flags) == 0,
			"autorequest GPIO-%d\n", gpio)) {
		if (!try_module_get(chip->owner)) {
			pr_err("GPIO-%d: module can't be gotten \n", gpio);
			clear_bit(FLAG_REQUESTED, &desc->flags);
			 
			return -EIO;
		}
		desc_set_label(desc, "[auto]");
		 
		if (chip->request)
			return 1;
	}
	return 0;
}

static inline struct gpio_chip *gpio_to_chip(unsigned gpio)
{
	return gpio_desc[gpio].chip;
}

static int gpiochip_find_base(int ngpio)
{
	int i;
	int spare = 0;
	int base = -ENOSPC;

	for (i = ARCH_NR_GPIOS - 1; i >= 0 ; i--) {
		struct gpio_desc *desc = &gpio_desc[i];
		struct gpio_chip *chip = desc->chip;

		if (!chip && !test_bit(FLAG_RESERVED, &desc->flags)) {
			spare++;
			if (spare == ngpio) {
				base = i;
				break;
			}
		} else {
			spare = 0;
			if (chip)
				i -= chip->ngpio - 1;
		}
	}

	if (gpio_is_valid(base))
		pr_debug("%s: found new base at %d\n", __func__, base);
	return base;
}

int __init gpiochip_reserve(int start, int ngpio)
{
	int ret = 0;
	unsigned long flags;
	int i;

	if (!gpio_is_valid(start) || !gpio_is_valid(start + ngpio - 1))
		return -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	for (i = start; i < start + ngpio; i++) {
		struct gpio_desc *desc = &gpio_desc[i];

		if (desc->chip || test_bit(FLAG_RESERVED, &desc->flags)) {
			ret = -EBUSY;
			goto err;
		}

		set_bit(FLAG_RESERVED, &desc->flags);
	}

	pr_debug("%s: reserved gpios from %d to %d\n",
		 __func__, start, start + ngpio - 1);
err:
	spin_unlock_irqrestore(&gpio_lock, flags);

	return ret;
}

#ifdef CONFIG_GPIO_SYSFS

static DEFINE_MUTEX(sysfs_lock);

static ssize_t gpio_direction_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else
		status = sprintf(buf, "%s\n",
			test_bit(FLAG_IS_OUT, &desc->flags)
				? "out" : "in");

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t gpio_direction_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	unsigned		gpio = desc - gpio_desc;
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else if (sysfs_streq(buf, "high"))
		status = gpio_direction_output(gpio, 1);
	else if (sysfs_streq(buf, "out") || sysfs_streq(buf, "low"))
		status = gpio_direction_output(gpio, 0);
	else if (sysfs_streq(buf, "in"))
		status = gpio_direction_input(gpio);
	else
		status = -EINVAL;

	mutex_unlock(&sysfs_lock);
	return status ? : size;
}

static const DEVICE_ATTR(direction, 0644,
		gpio_direction_show, gpio_direction_store);

static ssize_t gpio_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	unsigned		gpio = desc - gpio_desc;
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else
		status = sprintf(buf, "%d\n", !!gpio_get_value_cansleep(gpio));

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t gpio_value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	unsigned		gpio = desc - gpio_desc;
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else if (!test_bit(FLAG_IS_OUT, &desc->flags))
		status = -EPERM;
	else {
		long		value;

		status = strict_strtol(buf, 0, &value);
		if (status == 0) {
			gpio_set_value_cansleep(gpio, value != 0);
			status = size;
		}
	}

	mutex_unlock(&sysfs_lock);
	return status;
}

static   DEVICE_ATTR(value, 0644,
		gpio_value_show, gpio_value_store);

static irqreturn_t gpio_sysfs_irq(int irq, void *priv)
{
	struct work_struct	*work = priv;

	schedule_work(work);
	return IRQ_HANDLED;
}

static void gpio_notify_sysfs(struct work_struct *work)
{
	struct poll_desc	*pdesc;

	pdesc = container_of(work, struct poll_desc, work);
	sysfs_notify_dirent(pdesc->value_sd);
}

static int gpio_setup_irq(struct gpio_desc *desc, struct device *dev,
		unsigned long gpio_flags)
{
	struct poll_desc	*pdesc;
	unsigned long		irq_flags;
	int			ret, irq, id;

	if ((desc->flags & GPIO_TRIGGER_MASK) == gpio_flags)
		return 0;

	irq = gpio_to_irq(desc - gpio_desc);
	if (irq < 0)
		return -EIO;

	id = desc->flags >> PDESC_ID_SHIFT;
	pdesc = idr_find(&pdesc_idr, id);
	if (pdesc) {
		free_irq(irq, &pdesc->work);
		cancel_work_sync(&pdesc->work);
	}

	desc->flags &= ~GPIO_TRIGGER_MASK;

	if (!gpio_flags) {
		ret = 0;
		goto free_sd;
	}

	irq_flags = IRQF_SHARED;
	if (test_bit(FLAG_TRIG_FALL, &gpio_flags))
		irq_flags |= IRQF_TRIGGER_FALLING;
	if (test_bit(FLAG_TRIG_RISE, &gpio_flags))
		irq_flags |= IRQF_TRIGGER_RISING;

	if (!pdesc) {
		pdesc = kmalloc(sizeof(*pdesc), GFP_KERNEL);
		if (!pdesc) {
			ret = -ENOMEM;
			goto err_out;
		}

		do {
			ret = -ENOMEM;
			if (idr_pre_get(&pdesc_idr, GFP_KERNEL))
				ret = idr_get_new_above(&pdesc_idr,
						pdesc, 1, &id);
		} while (ret == -EAGAIN);

		if (ret)
			goto free_mem;

		desc->flags &= GPIO_FLAGS_MASK;
		desc->flags |= (unsigned long)id << PDESC_ID_SHIFT;

		if (desc->flags >> PDESC_ID_SHIFT != id) {
			ret = -ERANGE;
			goto free_id;
		}

		pdesc->value_sd = sysfs_get_dirent(dev->kobj.sd, "value");
		if (!pdesc->value_sd) {
			ret = -ENODEV;
			goto free_id;
		}
		INIT_WORK(&pdesc->work, gpio_notify_sysfs);
	}

	ret = request_irq(irq, gpio_sysfs_irq, irq_flags,
			"gpiolib", &pdesc->work);
	if (ret)
		goto free_sd;

	desc->flags |= gpio_flags;
	return 0;

free_sd:
	sysfs_put(pdesc->value_sd);
free_id:
	idr_remove(&pdesc_idr, id);
	desc->flags &= GPIO_FLAGS_MASK;
free_mem:
	kfree(pdesc);
err_out:
	return ret;
}

static const struct {
	const char *name;
	unsigned long flags;
} trigger_types[] = {
	{ "none",    0 },
	{ "falling", BIT(FLAG_TRIG_FALL) },
	{ "rising",  BIT(FLAG_TRIG_RISE) },
	{ "both",    BIT(FLAG_TRIG_FALL) | BIT(FLAG_TRIG_RISE) },
};

static ssize_t gpio_edge_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else {
		int i;

		status = 0;
		for (i = 0; i < ARRAY_SIZE(trigger_types); i++)
			if ((desc->flags & GPIO_TRIGGER_MASK)
					== trigger_types[i].flags) {
				status = sprintf(buf, "%s\n",
						 trigger_types[i].name);
				break;
			}
	}

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t gpio_edge_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t			status;
	int			i;

	for (i = 0; i < ARRAY_SIZE(trigger_types); i++)
		if (sysfs_streq(trigger_types[i].name, buf))
			goto found;
	return -EINVAL;

found:
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else {
		status = gpio_setup_irq(desc, dev, trigger_types[i].flags);
		if (!status)
			status = size;
	}

	mutex_unlock(&sysfs_lock);

	return status;
}

static DEVICE_ATTR(edge, 0644, gpio_edge_show, gpio_edge_store);

static const struct attribute *gpio_attrs[] = {
	&dev_attr_direction.attr,
	&dev_attr_value.attr,
	NULL,
};

static const struct attribute_group gpio_attr_group = {
	.attrs = (struct attribute **) gpio_attrs,
};

static ssize_t chip_base_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", chip->base);
}
static DEVICE_ATTR(base, 0444, chip_base_show, NULL);

static ssize_t chip_label_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", chip->label ? : "");
}
static DEVICE_ATTR(label, 0444, chip_label_show, NULL);

static ssize_t chip_ngpio_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", chip->ngpio);
}
static DEVICE_ATTR(ngpio, 0444, chip_ngpio_show, NULL);

static const struct attribute *gpiochip_attrs[] = {
	&dev_attr_base.attr,
	&dev_attr_label.attr,
	&dev_attr_ngpio.attr,
	NULL,
};

static const struct attribute_group gpiochip_attr_group = {
	.attrs = (struct attribute **) gpiochip_attrs,
};

static ssize_t export_store(struct class *class, const char *buf, size_t len)
{
	long	gpio;
	int	status;

	status = strict_strtol(buf, 0, &gpio);
	if (status < 0)
		goto done;

	status = gpio_request(gpio, "sysfs");
	if (status < 0)
		goto done;

	status = gpio_export(gpio, true);
	if (status < 0)
		gpio_free(gpio);
	else
		set_bit(FLAG_SYSFS, &gpio_desc[gpio].flags);

done:
	if (status)
		pr_debug("%s: status %d\n", __func__, status);
	return status ? : len;
}

static ssize_t unexport_store(struct class *class, const char *buf, size_t len)
{
	long	gpio;
	int	status;

	status = strict_strtol(buf, 0, &gpio);
	if (status < 0)
		goto done;

	status = -EINVAL;

	if (!gpio_is_valid(gpio))
		goto done;

	if (test_and_clear_bit(FLAG_SYSFS, &gpio_desc[gpio].flags)) {
		status = 0;
		gpio_free(gpio);
	}
done:
	if (status)
		pr_debug("%s: status %d\n", __func__, status);
	return status ? : len;
}

static struct class_attribute gpio_class_attrs[] = {
	__ATTR(export, 0200, NULL, export_store),
	__ATTR(unexport, 0200, NULL, unexport_store),
	__ATTR_NULL,
};

static struct class gpio_class = {
	.name =		"gpio",
	.owner =	THIS_MODULE,

	.class_attrs =	gpio_class_attrs,
};

int gpio_export(unsigned gpio, bool direction_may_change)
{
	unsigned long		flags;
	struct gpio_desc	*desc;
	int			status = -EINVAL;
	char			*ioname = NULL;

	if (!gpio_class.p) {
		pr_debug("%s: called too early!\n", __func__);
		return -ENOENT;
	}

	if (!gpio_is_valid(gpio))
		goto done;

	mutex_lock(&sysfs_lock);

	spin_lock_irqsave(&gpio_lock, flags);
	desc = &gpio_desc[gpio];
	if (test_bit(FLAG_REQUESTED, &desc->flags)
			&& !test_bit(FLAG_EXPORT, &desc->flags)) {
		status = 0;
		if (!desc->chip->direction_input
				|| !desc->chip->direction_output)
			direction_may_change = false;
	}
	spin_unlock_irqrestore(&gpio_lock, flags);

	if (desc->chip->names && desc->chip->names[gpio - desc->chip->base])
		ioname = desc->chip->names[gpio - desc->chip->base];

	if (status == 0) {
		struct device	*dev;

		dev = device_create(&gpio_class, desc->chip->dev, MKDEV(0, 0),
				desc, ioname ? ioname : "gpio%d", gpio);
		if (!IS_ERR(dev)) {
			if (direction_may_change)
				status = sysfs_create_group(&dev->kobj,
						&gpio_attr_group);
			else
				status = device_create_file(dev,
						&dev_attr_value);

			if (!status && gpio_to_irq(gpio) >= 0
					&& (direction_may_change
						|| !test_bit(FLAG_IS_OUT,
							&desc->flags)))
				status = device_create_file(dev,
						&dev_attr_edge);

			if (status != 0)
				device_unregister(dev);
		} else
			status = PTR_ERR(dev);
		if (status == 0)
			set_bit(FLAG_EXPORT, &desc->flags);
	}

	mutex_unlock(&sysfs_lock);

done:
	if (status)
		pr_debug("%s: gpio%d status %d\n", __func__, gpio, status);

	return status;
}
EXPORT_SYMBOL_GPL(gpio_export);

static int match_export(struct device *dev, void *data)
{
	return dev_get_drvdata(dev) == data;
}

int gpio_export_link(struct device *dev, const char *name, unsigned gpio)
{
	struct gpio_desc	*desc;
	int			status = -EINVAL;

	if (!gpio_is_valid(gpio))
		goto done;

	mutex_lock(&sysfs_lock);

	desc = &gpio_desc[gpio];

	if (test_bit(FLAG_EXPORT, &desc->flags)) {
		struct device *tdev;

		tdev = class_find_device(&gpio_class, NULL, desc, match_export);
		if (tdev != NULL) {
			status = sysfs_create_link(&dev->kobj, &tdev->kobj,
						name);
		} else {
			status = -ENODEV;
		}
	}

	mutex_unlock(&sysfs_lock);

done:
	if (status)
		pr_debug("%s: gpio%d status %d\n", __func__, gpio, status);

	return status;
}
EXPORT_SYMBOL_GPL(gpio_export_link);

void gpio_unexport(unsigned gpio)
{
	struct gpio_desc	*desc;
	int			status = -EINVAL;

	if (!gpio_is_valid(gpio))
		goto done;

	mutex_lock(&sysfs_lock);

	desc = &gpio_desc[gpio];

	if (test_bit(FLAG_EXPORT, &desc->flags)) {
		struct device	*dev = NULL;

		dev = class_find_device(&gpio_class, NULL, desc, match_export);
		if (dev) {
			gpio_setup_irq(desc, dev, 0);
			clear_bit(FLAG_EXPORT, &desc->flags);
			put_device(dev);
			device_unregister(dev);
			status = 0;
		} else
			status = -ENODEV;
	}

	mutex_unlock(&sysfs_lock);
done:
	if (status)
		pr_debug("%s: gpio%d status %d\n", __func__, gpio, status);
}
EXPORT_SYMBOL_GPL(gpio_unexport);

static int gpiochip_export(struct gpio_chip *chip)
{
	int		status;
	struct device	*dev;

	if (!gpio_class.p)
		return 0;

	mutex_lock(&sysfs_lock);
	dev = device_create(&gpio_class, chip->dev, MKDEV(0, 0), chip,
				"gpiochip%d", chip->base);
	if (!IS_ERR(dev)) {
		status = sysfs_create_group(&dev->kobj,
				&gpiochip_attr_group);
	} else
		status = PTR_ERR(dev);
	chip->exported = (status == 0);
	mutex_unlock(&sysfs_lock);

	if (status) {
		unsigned long	flags;
		unsigned	gpio;

		spin_lock_irqsave(&gpio_lock, flags);
		gpio = chip->base;
		while (gpio_desc[gpio].chip == chip)
			gpio_desc[gpio++].chip = NULL;
		spin_unlock_irqrestore(&gpio_lock, flags);

		pr_debug("%s: chip %s status %d\n", __func__,
				chip->label, status);
	}

	return status;
}

static void gpiochip_unexport(struct gpio_chip *chip)
{
	int			status;
	struct device		*dev;

	mutex_lock(&sysfs_lock);
	dev = class_find_device(&gpio_class, NULL, chip, match_export);
	if (dev) {
		put_device(dev);
		device_unregister(dev);
		chip->exported = 0;
		status = 0;
	} else
		status = -ENODEV;
	mutex_unlock(&sysfs_lock);

	if (status)
		pr_debug("%s: chip %s status %d\n", __func__,
				chip->label, status);
}

static int __init gpiolib_sysfs_init(void)
{
	int		status;
	unsigned long	flags;
	unsigned	gpio;

	idr_init(&pdesc_idr);

	status = class_register(&gpio_class);
	if (status < 0)
		return status;

	spin_lock_irqsave(&gpio_lock, flags);
	for (gpio = 0; gpio < ARCH_NR_GPIOS; gpio++) {
		struct gpio_chip	*chip;

		chip = gpio_desc[gpio].chip;
		if (!chip || chip->exported)
			continue;

		spin_unlock_irqrestore(&gpio_lock, flags);
		status = gpiochip_export(chip);
		spin_lock_irqsave(&gpio_lock, flags);
	}
	spin_unlock_irqrestore(&gpio_lock, flags);

	return status;
}
postcore_initcall(gpiolib_sysfs_init);

#else
static inline int gpiochip_export(struct gpio_chip *chip)
{
	return 0;
}

static inline void gpiochip_unexport(struct gpio_chip *chip)
{
}

#endif  

int gpiochip_add(struct gpio_chip *chip)
{
	unsigned long	flags;
	int		status = 0;
	unsigned	id;
	int		base = chip->base;

	if ((!gpio_is_valid(base) || !gpio_is_valid(base + chip->ngpio - 1))
			&& base >= 0) {
		status = -EINVAL;
		goto fail;
	}

	spin_lock_irqsave(&gpio_lock, flags);

	if (base < 0) {
		base = gpiochip_find_base(chip->ngpio);
		if (base < 0) {
			status = base;
			goto unlock;
		}
		chip->base = base;
	}

	for (id = base; id < base + chip->ngpio; id++) {
		if (gpio_desc[id].chip != NULL) {
			status = -EBUSY;
			break;
		}
	}
	if (status == 0) {
		for (id = base; id < base + chip->ngpio; id++) {
			gpio_desc[id].chip = chip;

			gpio_desc[id].flags = !chip->direction_input
				? (1 << FLAG_IS_OUT)
				: 0;
		}
	}

unlock:
	spin_unlock_irqrestore(&gpio_lock, flags);
	if (status == 0)
		status = gpiochip_export(chip);
fail:
	 
	if (status)
		pr_err("gpiochip_add: gpios %d..%d (%s) not registered\n",
			chip->base, chip->base + chip->ngpio - 1,
			chip->label ? : "generic");
	return status;
}
EXPORT_SYMBOL_GPL(gpiochip_add);

int gpiochip_remove(struct gpio_chip *chip)
{
	unsigned long	flags;
	int		status = 0;
	unsigned	id;

	spin_lock_irqsave(&gpio_lock, flags);

	for (id = chip->base; id < chip->base + chip->ngpio; id++) {
		if (test_bit(FLAG_REQUESTED, &gpio_desc[id].flags)) {
			status = -EBUSY;
			break;
		}
	}
	if (status == 0) {
		for (id = chip->base; id < chip->base + chip->ngpio; id++)
			gpio_desc[id].chip = NULL;
	}

	spin_unlock_irqrestore(&gpio_lock, flags);

	if (status == 0)
		gpiochip_unexport(chip);

	return status;
}
EXPORT_SYMBOL_GPL(gpiochip_remove);

int gpio_request(unsigned gpio, const char *label)
{
	struct gpio_desc	*desc;
	struct gpio_chip	*chip;
	int			status = -EINVAL;
	unsigned long		flags;

	spin_lock_irqsave(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		goto done;
	desc = &gpio_desc[gpio];
	chip = desc->chip;
	if (chip == NULL)
		goto done;

	if (!try_module_get(chip->owner))
		goto done;

	if (test_and_set_bit(FLAG_REQUESTED, &desc->flags) == 0) {
		desc_set_label(desc, label ? : "?");
		status = 0;
	} else {
		status = -EBUSY;
		module_put(chip->owner);
		goto done;
	}

	if (chip->request) {
		 
		spin_unlock_irqrestore(&gpio_lock, flags);
		status = chip->request(chip, gpio - chip->base);
		spin_lock_irqsave(&gpio_lock, flags);

		if (status < 0) {
			desc_set_label(desc, NULL);
			module_put(chip->owner);
			clear_bit(FLAG_REQUESTED, &desc->flags);
		}
	}

done:
	if (status)
		pr_debug("gpio_request: gpio-%d (%s) status %d\n",
			gpio, label ? : "?", status);
	spin_unlock_irqrestore(&gpio_lock, flags);
	return status;
}
EXPORT_SYMBOL_GPL(gpio_request);

void gpio_free(unsigned gpio)
{
	unsigned long		flags;
	struct gpio_desc	*desc;
	struct gpio_chip	*chip;

	might_sleep();

	if (!gpio_is_valid(gpio)) {
		WARN_ON(extra_checks);
		return;
	}

	gpio_unexport(gpio);

	spin_lock_irqsave(&gpio_lock, flags);

	desc = &gpio_desc[gpio];
	chip = desc->chip;
	if (chip && test_bit(FLAG_REQUESTED, &desc->flags)) {
		if (chip->free) {
			spin_unlock_irqrestore(&gpio_lock, flags);
			might_sleep_if(extra_checks && chip->can_sleep);
			chip->free(chip, gpio - chip->base);
			spin_lock_irqsave(&gpio_lock, flags);
		}
		desc_set_label(desc, NULL);
		module_put(desc->chip->owner);
		clear_bit(FLAG_REQUESTED, &desc->flags);
	} else
		WARN_ON(extra_checks);

	spin_unlock_irqrestore(&gpio_lock, flags);
}
EXPORT_SYMBOL_GPL(gpio_free);

const char *gpiochip_is_requested(struct gpio_chip *chip, unsigned offset)
{
	unsigned gpio = chip->base + offset;

	if (!gpio_is_valid(gpio) || gpio_desc[gpio].chip != chip)
		return NULL;
	if (test_bit(FLAG_REQUESTED, &gpio_desc[gpio].flags) == 0)
		return NULL;
#ifdef CONFIG_DEBUG_FS
	return gpio_desc[gpio].label;
#else
	return "?";
#endif
}
EXPORT_SYMBOL_GPL(gpiochip_is_requested);

int gpio_direction_input(unsigned gpio)
{
	unsigned long		flags;
	struct gpio_chip	*chip;
	struct gpio_desc	*desc = &gpio_desc[gpio];
	int			status = -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		goto fail;
	chip = desc->chip;
	if (!chip || !chip->get || !chip->direction_input)
		goto fail;
	gpio -= chip->base;
	if (gpio >= chip->ngpio)
		goto fail;
	status = gpio_ensure_requested(desc, gpio);
	if (status < 0)
		goto fail;

	spin_unlock_irqrestore(&gpio_lock, flags);

	might_sleep_if(extra_checks && chip->can_sleep);

	if (status) {
		status = chip->request(chip, gpio);
		if (status < 0) {
			pr_debug("GPIO-%d: chip request fail, %d\n",
				chip->base + gpio, status);
			 
			goto lose;
		}
	}

	status = chip->direction_input(chip, gpio);
	if (status == 0)
		clear_bit(FLAG_IS_OUT, &desc->flags);
lose:
	return status;
fail:
	spin_unlock_irqrestore(&gpio_lock, flags);
	if (status)
		pr_debug("%s: gpio-%d status %d\n",
			__func__, gpio, status);
	return status;
}
EXPORT_SYMBOL_GPL(gpio_direction_input);

int gpio_direction_output(unsigned gpio, int value)
{
	unsigned long		flags;
	struct gpio_chip	*chip;
	struct gpio_desc	*desc = &gpio_desc[gpio];
	int			status = -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		goto fail;
	chip = desc->chip;
	if (!chip || !chip->set || !chip->direction_output)
		goto fail;
	gpio -= chip->base;
	if (gpio >= chip->ngpio)
		goto fail;
	status = gpio_ensure_requested(desc, gpio);
	if (status < 0)
		goto fail;

	spin_unlock_irqrestore(&gpio_lock, flags);

	might_sleep_if(extra_checks && chip->can_sleep);

	if (status) {
		status = chip->request(chip, gpio);
		if (status < 0) {
			pr_debug("GPIO-%d: chip request fail, %d\n",
				chip->base + gpio, status);
			 
			goto lose;
		}
	}

	status = chip->direction_output(chip, gpio, value);
	if (status == 0)
		set_bit(FLAG_IS_OUT, &desc->flags);
lose:
	return status;
fail:
	spin_unlock_irqrestore(&gpio_lock, flags);
	if (status)
		pr_debug("%s: gpio-%d status %d\n",
			__func__, gpio, status);
	return status;
}
EXPORT_SYMBOL_GPL(gpio_direction_output);

int __gpio_get_value(unsigned gpio)
{
	struct gpio_chip	*chip;

	chip = gpio_to_chip(gpio);
	WARN_ON(extra_checks && chip->can_sleep);
	return chip->get ? chip->get(chip, gpio - chip->base) : 0;
}
EXPORT_SYMBOL_GPL(__gpio_get_value);

void __gpio_set_value(unsigned gpio, int value)
{
	struct gpio_chip	*chip;

	chip = gpio_to_chip(gpio);
	WARN_ON(extra_checks && chip->can_sleep);
	chip->set(chip, gpio - chip->base, value);
}
EXPORT_SYMBOL_GPL(__gpio_set_value);

int __gpio_cansleep(unsigned gpio)
{
	struct gpio_chip	*chip;

	chip = gpio_to_chip(gpio);

	return chip->can_sleep;
}
EXPORT_SYMBOL_GPL(__gpio_cansleep);

int __gpio_to_irq(unsigned gpio)
{
	struct gpio_chip	*chip;

	chip = gpio_to_chip(gpio);
	return chip->to_irq ? chip->to_irq(chip, gpio - chip->base) : -ENXIO;
}
EXPORT_SYMBOL_GPL(__gpio_to_irq);

#ifdef MY_DEF_HERE
int gpio_hw_reset(unsigned gpio)
{
	struct gpio_chip	*chip;
	int iRet =-1;

	chip = gpio_to_chip(gpio);
	WARN_ON(extra_checks && chip->can_sleep);
	if(!chip->iHWReset) {
		goto END;
	}

	chip->iHWReset(chip, gpio - chip->base);

	iRet = 0;

END:
	return iRet;
}
EXPORT_SYMBOL_GPL(gpio_hw_reset);
#endif

#ifdef MY_DEF_HERE
 
int iGpioInterruptClear(unsigned int gpio)
{
	struct gpio_chip	*chip;

	chip = gpio_to_chip(gpio);
	WARN_ON(extra_checks && chip->can_sleep);
	return chip->iInterruptClear ? chip->iInterruptClear(chip, gpio - chip->base) : 0;
}
EXPORT_SYMBOL_GPL(iGpioInterruptClear);
#endif

int gpio_get_value_cansleep(unsigned gpio)
{
	struct gpio_chip	*chip;

	might_sleep_if(extra_checks);
	chip = gpio_to_chip(gpio);
	return chip->get ? chip->get(chip, gpio - chip->base) : 0;
}
EXPORT_SYMBOL_GPL(gpio_get_value_cansleep);

void gpio_set_value_cansleep(unsigned gpio, int value)
{
	struct gpio_chip	*chip;

	might_sleep_if(extra_checks);
	chip = gpio_to_chip(gpio);
	chip->set(chip, gpio - chip->base, value);
}
EXPORT_SYMBOL_GPL(gpio_set_value_cansleep);

#ifdef CONFIG_DEBUG_FS

static void gpiolib_dbg_show(struct seq_file *s, struct gpio_chip *chip)
{
	unsigned		i;
	unsigned		gpio = chip->base;
	struct gpio_desc	*gdesc = &gpio_desc[gpio];
	int			is_out;

	for (i = 0; i < chip->ngpio; i++, gpio++, gdesc++) {
		if (!test_bit(FLAG_REQUESTED, &gdesc->flags))
			continue;

		is_out = test_bit(FLAG_IS_OUT, &gdesc->flags);
		seq_printf(s, " gpio-%-3d (%-20.20s) %s %s",
			gpio, gdesc->label,
			is_out ? "out" : "in ",
			chip->get
				? (chip->get(chip, i) ? "hi" : "lo")
				: "?  ");

		if (!is_out) {
			int		irq = gpio_to_irq(gpio);
			struct irq_desc	*desc = irq_to_desc(irq);

			if (irq >= 0 && desc->action) {
				char *trigger;

				switch (desc->status & IRQ_TYPE_SENSE_MASK) {
				case IRQ_TYPE_NONE:
					trigger = "(default)";
					break;
				case IRQ_TYPE_EDGE_FALLING:
					trigger = "edge-falling";
					break;
				case IRQ_TYPE_EDGE_RISING:
					trigger = "edge-rising";
					break;
				case IRQ_TYPE_EDGE_BOTH:
					trigger = "edge-both";
					break;
				case IRQ_TYPE_LEVEL_HIGH:
					trigger = "level-high";
					break;
				case IRQ_TYPE_LEVEL_LOW:
					trigger = "level-low";
					break;
				default:
					trigger = "?trigger?";
					break;
				}

				seq_printf(s, " irq-%d %s%s",
					irq, trigger,
					(desc->status & IRQ_WAKEUP)
						? " wakeup" : "");
			}
		}

		seq_printf(s, "\n");
	}
}

static int gpiolib_show(struct seq_file *s, void *unused)
{
	struct gpio_chip	*chip = NULL;
	unsigned		gpio;
	int			started = 0;

	for (gpio = 0; gpio_is_valid(gpio); gpio++) {
		struct device *dev;

		if (chip == gpio_desc[gpio].chip)
			continue;
		chip = gpio_desc[gpio].chip;
		if (!chip)
			continue;

		seq_printf(s, "%sGPIOs %d-%d",
				started ? "\n" : "",
				chip->base, chip->base + chip->ngpio - 1);
		dev = chip->dev;
		if (dev)
			seq_printf(s, ", %s/%s",
				dev->bus ? dev->bus->name : "no-bus",
				dev_name(dev));
		if (chip->label)
			seq_printf(s, ", %s", chip->label);
		if (chip->can_sleep)
			seq_printf(s, ", can sleep");
		seq_printf(s, ":\n");

		started = 1;
		if (chip->dbg_show)
			chip->dbg_show(s, chip);
		else
			gpiolib_dbg_show(s, chip);
	}
	return 0;
}

static int gpiolib_open(struct inode *inode, struct file *file)
{
	return single_open(file, gpiolib_show, NULL);
}

static const struct file_operations gpiolib_operations = {
	.open		= gpiolib_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init gpiolib_debugfs_init(void)
{
	 
	(void) debugfs_create_file("gpio", S_IFREG | S_IRUGO,
				NULL, NULL, &gpiolib_operations);
	return 0;
}
subsys_initcall(gpiolib_debugfs_init);

#endif	 
