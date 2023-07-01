/*
 *  linux/arch/arm/mach-comcerto/gpio.c
 *
 *  Copyright (C) 2006 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* [FIXME] */
#if 0
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <linux/kernel_stat.h>

#if !defined(CONFIG_ARCH_M83XXX)

/* The GPIO IRQ block generates interrupts only on rising/falling edges of the GPIO pin signal.
 * To avoid loosing interrupts or having spurious interrupts care must be taken.
 * The general strategy is to loop and poll the GPIO pin to make sure no interrupts are missed.
 * The GPIO IRQ must be acked inside the loop at each iteration. If it was acked
 * before the loop there would be a race condition(1) where we exit comcerto_handle_gpio_level_irq() with
 * the GPIO IRQ set, even if the source was already handled. If it was acked after the loop
 * there would be a race condition(2) where we ack a GPIO IRQ but the source is not yet handled.
 * The GPIO IRQ must be acked after all the driver handlers have been called (after handle_simple_irq())
 * to also avoid the race mentioned in (1) above.
 */

extern int noirqdebug;
extern int redirect_hardirq(struct irq_desc *desc);

void comcerto_handle_gpio_level_irq(unsigned int irq, struct irq_desc *desc)
{
	struct irqaction *action;
	irqreturn_t action_ret;
	const unsigned int cpu = smp_processor_id();
	u32 pending;

	spin_lock(&desc->lock);

	/*
	 * Mask IRQ.
	 */
	desc->chip->mask(irq);

	do {
		if (unlikely(desc->status & IRQ_INPROGRESS))
			goto out_unlock;
		desc->status &= ~(IRQ_REPLAY | IRQ_WAITING);
		kstat_cpu(cpu).irqs[irq]++;

		action = desc->action;
		if (unlikely(!action || (desc->status & IRQ_DISABLED))) {
			desc->status |= IRQ_PENDING;
			goto out_unlock;
		}

		desc->status |= IRQ_INPROGRESS;
		/*
		 * hardirq redirection to the irqd process context:
		 */
		if (redirect_hardirq(desc))
			goto out_unlock;
		desc->status &= ~IRQ_PENDING;
		spin_unlock(&desc->lock);

		action_ret = handle_IRQ_event(irq, action);
		if (!noirqdebug)
			note_interrupt(irq, desc, action_ret);

		spin_lock(&desc->lock);
		desc->status &= ~IRQ_INPROGRESS;

		/*
		 * Ack IRQ.
		 */
		desc->chip->ack(irq);
		/*
		 * Source interrupts are usually active low
		 */
		pending = comcerto_gpio_read(1 << ((irq - 1) & 0x1f)) ? 0 : 1;

	} while (pending && !(desc->status & IRQ_DISABLED));

	if (!(desc->status & IRQ_DISABLED) && desc->chip->unmask)
		desc->chip->unmask(irq);

out_unlock:
	spin_unlock(&desc->lock);
}
#endif
#endif
