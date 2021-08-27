/*
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_GPIO_H__
#define __MACH_GPIO_H__

#include <asm/errno.h>
#include <mach/armada375.h>
#include <mach/irqs.h>
#include <plat/gpio.h>
#include <asm-generic/gpio.h>

#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep

#define GPIO_MAX	64

#define GPIO_BASE_LO		(GPIO_VIRT_BASE + 0x00)
#define GPIO_BASE_HI		(GPIO_VIRT_BASE + 0x20)

#define GPIO_BASE(pin)		((pin < 32) ? GPIO_BASE_LO : GPIO_BASE_HI)

#define GPIO_OUT(pin)		(GPIO_BASE(pin) + 0x00)
#define GPIO_IO_CONF(pin)	(GPIO_BASE(pin) + 0x04)
#define GPIO_BLINK_EN(pin)	(GPIO_BASE(pin) + 0x08)
#define GPIO_IN_POL(pin)	(GPIO_BASE(pin) + 0x0c)
#define GPIO_DATA_IN(pin)	(GPIO_BASE(pin) + 0x10)
#define GPIO_EDGE_CAUSE(pin)	(GPIO_BASE(pin) + 0x14)
#define GPIO_EDGE_MASK(pin)	(GPIO_BASE(pin) + 0x18)
#define GPIO_LEVEL_MASK(pin)	(GPIO_BASE(pin) + 0x1c)

static inline int gpio_to_irq(int pin)
{
	if (pin < NR_IRQS_GLOBAL_GPIO)
		return pin + IRQ_START_GLOBAL_GPIO;

	WARN(1, "%s: Error: wrong gpio pin number (%d)\n", __func__, pin);
	return -EINVAL;
}

static inline int irq_to_gpio(int irq)
{
	if (irq >= IRQ_START_GLOBAL_GPIO &&
	    irq <  IRQ_START_GLOBAL_GPIO + NR_IRQS_GLOBAL_GPIO)
		return irq - IRQ_START_GLOBAL_GPIO;

	WARN(1, "%s: Error: wrong irq number (%d)\n", __func__, irq);
	return -EINVAL;
}

#endif /* __MACH_GPIO_H__ */
