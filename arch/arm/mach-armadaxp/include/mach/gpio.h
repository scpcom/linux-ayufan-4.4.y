/*
 * include/asm-arm/arch-dove/gpio.h
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#include <asm/errno.h>
#include <mach/irqs.h>
#include <plat/gpio.h>
#include <asm-generic/gpio.h>		/* cansleep wrappers */

#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep

#define GPIO_MAX	64

#define GPIO_BASE_LO		(AURORA_GPIO_VIRT_BASE + 0x00)
#define GPIO_BASE_HI		(AURORA_GPIO_VIRT_BASE + 0x20)

#define GPIO_BASE(pin)		((pin < 32) ? GPIO_BASE_LO : GPIO_BASE_HI)

static inline int gpio_to_irq(int pin)
{
	if (pin < NR_GPIO_IRQS)
		return pin + IRQ_AURORA_GPIO_START;

	return -EINVAL;
}

static inline int irq_to_gpio(int irq)
{
	if (IRQ_AURORA_GPIO_START < irq && irq < NR_IRQS)
		return irq - IRQ_AURORA_GPIO_START;

	return -EINVAL;
}

#endif
