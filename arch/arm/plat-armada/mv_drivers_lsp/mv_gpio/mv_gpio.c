/*
 * arch/arm/plat-feroceon/mv_drivers_lsp/mv_gpio/mv_gpio.c
 *
 * Marvell Feroceon SoC GPIO handling.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>

#include "mvCommon.h"
#include "mvOs.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"

static DEFINE_SPINLOCK(gpio_lock);

static inline void __set_direction(unsigned pin, int input)
{
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (input)
		mvGppTypeSet(grp, mask, MV_GPP_IN & mask);
	else
		mvGppTypeSet(grp, mask, MV_GPP_OUT & mask);
}

static void __set_level(unsigned pin, int high)
{
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (high)
		mvGppValueSet (grp, mask, mask);
	else
		mvGppValueSet (grp, mask, 0);
}

static inline void __set_blinking(unsigned pin, int blink)
{
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (blink)
		mvGppBlinkEn(grp, mask, mask);
	else
		mvGppBlinkEn(grp, mask, 0);
}

static inline int mv_gpio_is_valid(unsigned pin, int mode)
{
	return true;
}

/*
 * GENERIC_GPIO primitives.
 */
static int mv_gpio_direction_input(struct gpio_chip *chip, unsigned pin)
{
	unsigned long flags;

	if (!mv_gpio_is_valid(pin, GPIO_INPUT_OK))
		return -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	/* Configure GPIO direction. */
	__set_direction(pin, 1);

	spin_unlock_irqrestore(&gpio_lock, flags);

	return 0;
}

static int mv_gpio_get_value(struct gpio_chip *chip, unsigned pin)
{
	u32 val;
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (MV_REG_READ(GPP_DATA_OUT_EN_REG(grp)) & mask)
		val = mvGppValueGet(grp, mask) ^ mvGppPolarityGet(grp, mask);
	else
		val = MV_REG_READ(GPP_DATA_OUT_REG(grp));

	return (val >> (pin & 31)) & 1;
}

static int mv_gpio_direction_output(struct gpio_chip *chip, unsigned pin,
	int value)
{
	unsigned long flags;

	if (!mv_gpio_is_valid(pin, GPIO_OUTPUT_OK))
		return -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	/* Disable blinking. */
	__set_blinking(pin, 0);

	/* Configure GPIO output value. */
	__set_level(pin, value);

	/* Configure GPIO direction. */
	__set_direction(pin, 0);

	spin_unlock_irqrestore(&gpio_lock, flags);

	return 0;
}

static void mv_gpio_set_value(struct gpio_chip *chip, unsigned pin,
	int value)
{
	unsigned long flags;

	spin_lock_irqsave(&gpio_lock, flags);

	/* Configure GPIO output value. */
	__set_level(pin, value);

	spin_unlock_irqrestore(&gpio_lock, flags);
}

static int mv_gpio_request(struct gpio_chip *chip, unsigned pin)
{
	if (mv_gpio_is_valid(pin, GPIO_INPUT_OK) ||
	    mv_gpio_is_valid(pin, GPIO_OUTPUT_OK))
		return 0;
	return -EINVAL;
}

struct mv_gpio_regs {
	int data_out;
	int data_out_enable;
	int blink_enable;
	int data_in_polarity;
	int interrupt_mask;
	int interrupt_level_mask;
	int blink_cntr_select;
};

#define MV_GPP_MAX_REG_SET 	(CONFIG_MV_GPP_MAX_PINS/32)
static struct mv_gpio_regs regs[MV_GPP_MAX_REG_SET];

static struct gpio_chip mv_gpiochip = {
	.label			= "mv_gpio",
	.direction_input	= mv_gpio_direction_input,
	.get			= mv_gpio_get_value,
	.direction_output	= mv_gpio_direction_output,
	.set			= mv_gpio_set_value,
	.request		= mv_gpio_request,
	.base			= 0,
	.ngpio			= CONFIG_MV_GPP_MAX_PINS,
	.can_sleep		= 0,
};

static int mv_gpio_probe(struct platform_device *dev)
{
	gpiochip_add(&mv_gpiochip);

	return 0;
}

static int mv_gpio_suspend(struct platform_device *dev, pm_message_t state)
{
	int i;
	pr_info("Suspending GPIO\n");

	for(i = 0; i < MV_GPP_MAX_REG_SET; i++)
	{
		regs[i].data_out 	= MV_REG_READ(GPP_DATA_OUT_REG(i));
		regs[i].data_out_enable = MV_REG_READ(GPP_DATA_OUT_EN_REG(i));
		regs[i].blink_enable 	= MV_REG_READ(GPP_BLINK_EN_REG(i));
		regs[i].data_in_polarity = MV_REG_READ(GPP_DATA_IN_POL_REG(i));
		regs[i].interrupt_mask 	= MV_REG_READ(GPP_INT_MASK_REG(i));
		regs[i].interrupt_level_mask = MV_REG_READ(GPP_INT_LVL_REG(i));
		regs[i].blink_cntr_select = MV_REG_READ(GPP_BLINK_SEL_REG(i));
	}

	return 0;
}

static int mv_gpio_resume(struct platform_device *dev)
{
	int i;
	pr_info("Resuming GPIO\n");

	for(i = 0; i < MV_GPP_MAX_REG_SET; i++)
	{
		MV_REG_WRITE(GPP_DATA_OUT_REG(i), regs[i].data_out);
		MV_REG_WRITE(GPP_BLINK_EN_REG(i), regs[i].blink_enable);
		MV_REG_WRITE(GPP_DATA_IN_POL_REG(i), regs[i].data_in_polarity);
		MV_REG_WRITE(GPP_INT_MASK_REG(i), regs[i].interrupt_mask);
		MV_REG_WRITE(GPP_INT_LVL_REG(i), regs[i].interrupt_level_mask);
		MV_REG_WRITE(GPP_BLINK_SEL_REG(i), regs[i].blink_cntr_select);
		MV_REG_WRITE(GPP_DATA_OUT_EN_REG(i), regs[i].data_out_enable);
	}

	return 0;
}

static struct platform_driver mv_gpio_driver = {
	.probe    = mv_gpio_probe,
#ifdef CONFIG_PM
	.suspend = mv_gpio_suspend,
	.resume  = mv_gpio_resume,
#endif /* CONFIG_PM */
	.driver = {
		.name = "mv_gpio",
	},
};

static int __init mv_gpio_init_module(void)
{
	return platform_driver_register(&mv_gpio_driver);
}

module_init(mv_gpio_init_module);
MODULE_DESCRIPTION("Marvell GPIO Driver");
MODULE_LICENSE("GPL");
