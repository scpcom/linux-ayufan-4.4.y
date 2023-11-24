#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <mach/comcerto-common.h>
#include <asm/io.h>
#include <mach/gpio.h>

#define gpio_raw_readl(r) __raw_readl((void*)(r))
#define gpio_raw_writel(v,r) __raw_writel(v, (void*)(r))

#define DRV_NAME "c2k-gpio"
static DEFINE_SPINLOCK(c2k_gpio_lock);

static struct c2k_gpio_chip {
	struct gpio_chip chip;
};

static int c2k_is_gpio_rsvd(unsigned offset)
{
	if (offset < 32)
		return ((c2k_gpio_pin_stat.c2k_gpio_pins_0_31 >> offset) & 0x1) ? 1 : 0 ;
	else if (offset < 64)
		return ((c2k_gpio_pin_stat.c2k_gpio_pins_32_63 >> (offset - 32)) & 0x1) ? 1 : 0;
	else
		return -EINVAL;
}

static int c2k_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	if (offset < 32)
		return gpio_raw_readl(COMCERTO_GPIO_INPUT_REG) & (0x1 << offset);
	else if (offset < 64)
		return gpio_raw_readl(COMCERTO_GPIO_63_32_PIN_INPUT) & (0x1 << (offset - 32));
	else
		return -EINVAL;
}

static inline void __c2k_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	u32 data;

	if (offset < 32) {
		data = gpio_raw_readl(COMCERTO_GPIO_OUTPUT_REG);
		if (value)
			data |= (1 << offset);
		else
			data &= ~(1 << offset);
		gpio_raw_writel(data, COMCERTO_GPIO_OUTPUT_REG);
	} else {
		data = gpio_raw_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT);
		if (value)
			data |= (1 << (offset - 32));
		else
			data &= ~(1 << (offset - 32));
		gpio_raw_writel(data, COMCERTO_GPIO_63_32_PIN_OUTPUT);
	}
}

static void c2k_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned long flags;

	if (offset > 63)
		return;

	spin_lock_irqsave(&c2k_gpio_lock, flags);

	__c2k_gpio_set(chip, offset, value);

	spin_unlock_irqrestore(&c2k_gpio_lock, flags);
}

static int c2k_direction_input(struct gpio_chip *chip, unsigned offset)
{
	unsigned long flags;

	if (offset > 63)
		return -EINVAL;

	if (c2k_is_gpio_rsvd(offset)) {
		printk(KERN_ERR "GPIO-%d is reserved and cannot be used \n", offset);
		return -EINVAL;
	}

	spin_lock_irqsave(&c2k_gpio_lock, flags);

	if (offset < 32)
		gpio_raw_writel(gpio_raw_readl(COMCERTO_GPIO_OE_REG) & ~(0x1 << offset), COMCERTO_GPIO_OE_REG);
	else
		gpio_raw_writel(gpio_raw_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) | (0x1 << (offset - 32)), COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);

	spin_unlock_irqrestore(&c2k_gpio_lock, flags);

	return 0;
}

static int c2k_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned long flags;

	if (offset > 63)
		return -EINVAL;

	if (c2k_is_gpio_rsvd(offset)) {
		printk(KERN_ERR "GPIO-%d is reserved and cannot be used \n", offset);
		return -EINVAL;
	}

	spin_lock_irqsave(&c2k_gpio_lock, flags);

	if (offset < 32) {
		gpio_raw_writel(gpio_raw_readl(COMCERTO_GPIO_OE_REG) | (0x1 << offset), COMCERTO_GPIO_OE_REG);
		__c2k_gpio_set(chip, offset, value);
	} else {
		gpio_raw_writel(gpio_raw_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) & ~(0x1 << (offset - 32)), COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
		__c2k_gpio_set(chip, offset, value);
	}

	spin_unlock_irqrestore(&c2k_gpio_lock, flags);

	return 0;
}

static struct c2k_gpio_chip c2k_gpio_chip = {
	.chip = {
		.label			= DRV_NAME,
		.owner			= THIS_MODULE,
		.direction_input	= c2k_direction_input,
		.direction_output	= c2k_direction_output,
		.set			= c2k_gpio_set,
		.get			= c2k_gpio_get,
		.base			= 0,
		.ngpio			= C2K_GPIO_NR_GPIOS,
	},
};

int __init c2k_init_gpio(void)
{
	int ret;

	ret = gpiochip_add(&c2k_gpio_chip.chip);

	if (ret)
		printk(KERN_WARNING "C2K GPIO registration failed: %d\n", ret);

	return ret;
}
pure_initcall(c2k_init_gpio);

MODULE_AUTHOR("Mindspeed Technologies");
MODULE_DESCRIPTION("COMCERTO 2000 GPIO driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS(DRV_NAME);
