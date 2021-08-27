/*
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/hardware/gic.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include "ca9x2.h"
#include "mvOs.h"
#include "gpp/mvGppRegs.h"

#define SOC_PPI_CAUSE			0x21880
#define SOC_PPI_MASK_SET		0x218b8
#define SOC_PPI_MASK_CLEAR		0x218bc

static DEFINE_RAW_SPINLOCK(irq_controller_lock);

/*
 * Global GPIO interrupt handling
 */
static void a375_gpio_irq_mask(struct irq_data *d)
{
	u32 irq = d->irq, bitmask, reg;

	if (irq < IRQ_START_GLOBAL_GPIO ||
	    irq >= IRQ_START_GLOBAL_GPIO + NR_IRQS_GLOBAL_GPIO) {
		WARN(1, "Error: wrong GPIO irq %d\n", irq);
		return;		/* wrong  */
	}

	bitmask = 1 << (irq & 0x1F);
	reg = (irq - IRQ_START_GLOBAL_GPIO) >> 5;
	MV_REG_BIT_RESET(GPP_INT_LVL_REG(reg), bitmask);
}

static void a375_gpio_irq_unmask(struct irq_data *d)
{
	u32 irq = d->irq, bitmask, reg;

	if (irq < IRQ_START_GLOBAL_GPIO ||
	    irq >= IRQ_START_GLOBAL_GPIO + NR_IRQS_GLOBAL_GPIO) {
		WARN(1, "Error: wrong GPIO irq %d\n", irq);
		return;		/* wrong  */
	}

	bitmask = 1 << (irq & 0x1F);
	reg = (irq - IRQ_START_GLOBAL_GPIO) >> 5;
	MV_REG_BIT_SET(GPP_INT_LVL_REG(reg), bitmask);
}

static struct irq_chip a375_irq_chip = {
	.name = "a375_gpio_irq",
	.irq_mask = a375_gpio_irq_mask,
	.irq_mask_ack = a375_gpio_irq_mask,
	.irq_unmask = a375_gpio_irq_unmask,
	.irq_disable = a375_gpio_irq_mask,
	.irq_enable = a375_gpio_irq_unmask,
};

/*
 * gpio_cascade_irq is in [85:88] or [90:94]
 */
static void a375_gpio_cascade_irq_handler(unsigned int gpio_cascade_irq,
					  struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	u32 bit, n, gpio_group, irq;
	unsigned long cause, mask;

	chained_irq_enter(chip, desc);

	gpio_group = (gpio_cascade_irq - IRQ_GLOBAL_GPIO_0_7) / 4;

	cause = MV_REG_READ(GPP_INT_CAUSE_REG(gpio_group));
	mask = MV_REG_READ(GPP_INT_MASK_REG(gpio_group));
	cause &= mask;

	/* Ack GPIO interrupts */
	MV_REG_WRITE(GPP_INT_LVL_REG(gpio_group), cause);

	for_each_set_bit(bit, &cause, 32) {
		n = gpio_cascade_irq - IRQ_GLOBAL_GPIO_0_7;

		if (n >= 5)
			n -= 1;
		n = (n * 8) + (bit % 4);

		irq = IRQ_START_GLOBAL_GPIO + n;
		handle_level_irq(irq, desc);
	}

	chained_irq_exit(chip, desc);
}

static void __init a375_cascade_irq_gpio_global(void)
{
	int irq, irq_base;

	irq_base = irq_alloc_descs(-1, IRQ_START_GLOBAL_GPIO,
				   NR_IRQS_GLOBAL_GPIO, 0);
	if (IS_ERR_VALUE(irq_base))
		BUG();

	for (irq = IRQ_START_GLOBAL_GPIO;
	     irq < IRQ_START_GLOBAL_GPIO + NR_IRQS_GLOBAL_GPIO; irq++) {
		irq_set_chip_and_handler(irq, &a375_irq_chip, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	irq_set_chained_handler(IRQ_GLOBAL_GPIO_0_7,
				a375_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_8_15,
				a375_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_16_23,
				a375_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_24_31,
				a375_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_32_39,
				a375_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_40_47,
				a375_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_48_55,
				a375_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_56_63,
				a375_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_64_66,
				a375_gpio_cascade_irq_handler);
}

/*
 * Handle SOC PPI (Private Peripheral Interrupts).
 */
struct mpic_chip_regs {
	unsigned long cause;
	unsigned long mask;
	unsigned long unmask;
};

struct mpic_data {
	int irq_base;
	struct irq_domain *domain;
	struct mpic_chip_regs regs;
};

static struct mpic_data mpic_data = {
	.regs = {
		 .cause = INTER_REGS_VIRT_BASE + SOC_PPI_CAUSE,
		 .mask = INTER_REGS_VIRT_BASE + SOC_PPI_MASK_SET,
		 .unmask = INTER_REGS_VIRT_BASE + SOC_PPI_MASK_CLEAR,
		 }
};

static inline u_int
mpic_hw_irq(struct mpic_data *mpic, u_int irq)
{

	return irq - mpic->irq_base;
}

static void mpic_irq_mask(struct irq_data *d)
{
	struct mpic_data *mpic = irq_get_chip_data(d->irq);

	raw_spin_lock(&irq_controller_lock);
	writel_relaxed(mpic_hw_irq(mpic, d->irq), mpic->regs.mask);
	raw_spin_unlock(&irq_controller_lock);
}

static void mpic_irq_unmask(struct irq_data *d)
{
	struct mpic_data *mpic = irq_get_chip_data(d->irq);

	raw_spin_lock(&irq_controller_lock);
	writel_relaxed(mpic_hw_irq(mpic, d->irq), mpic->regs.unmask);
	raw_spin_unlock(&irq_controller_lock);
}

static struct irq_chip mpic_chip = {
	.irq_mask = mpic_irq_mask,
	.irq_unmask = mpic_irq_unmask,
};

static void mpic_handle_cascade_irq(unsigned int irq, struct irq_desc *desc)
{
	struct mpic_data *mpic = irq_get_handler_data(irq);
	struct irq_chip *chip = irq_get_chip(irq);
	unsigned long cause;

	chained_irq_enter(chip, desc);

	raw_spin_lock(&irq_controller_lock);
	cause = readl_relaxed(mpic->regs.cause);
	raw_spin_unlock(&irq_controller_lock);

	/* leave only relevant cause bits */
	cause &= 0x1FFAAFF;
	if (cause == 0)
		goto out;	/* spurious irq ? */

	irq = ffs(cause) - 1;
	generic_handle_irq(mpic->irq_base + irq);

out:
	chained_irq_exit(chip, desc);
}

static void __init a375_cascade_irq_mpic(int irq_start, int nr_irqs)
{
	struct mpic_data *mpic = &mpic_data;
	unsigned int i, intr;

	mpic->irq_base = irq_alloc_descs(-1, irq_start, nr_irqs, 0);
	if (IS_ERR_VALUE(mpic->irq_base))
		BUG();

	for (i = 0; i <= nr_irqs; i++) {
		intr = mpic->irq_base + i;
		irq_set_percpu_devid(intr);
		irq_set_chip_and_handler(intr, &mpic_chip, handle_percpu_devid_irq);
		irq_set_chip_data(intr, &mpic_data);
		set_irq_flags(intr, IRQF_VALID | IRQF_PROBE);
	}
	if (irq_set_handler_data(irq_start, &mpic_data) != 0)
		BUG();

	irq_set_chained_handler(irq_start, mpic_handle_cascade_irq);
}

static void __init a375_cascade_irq_gpio_private(void)
{
	/* TBD */
}

static void __init a375_cascade_irq_msi_global(void)
{
	/* TBD */
}

static void __init a375_cascade_irq_msi_private(void)
{
	/* TBD */
}

static void __init a375_cascade_irq_errors(void)
{
	/* TBD */
}

/*
 * Init GIC and MPIC and setup cascade irq
 * handling for GPIO, MSI and Error interrupts.
 */
void __init a375_init_irq(void)
{
	gic_init(0, 29,
		 (void __iomem *)(INTER_REGS_VIRT_BASE + A9_MPCORE_GIC_DIST),
		 (void __iomem *)(INTER_REGS_VIRT_BASE + A9_MPCORE_GIC_CPU));

	a375_cascade_irq_mpic(IRQ_START_PRIV_SOC_PPI, NR_IRQS_PRIV_SOC_PPI);

	a375_cascade_irq_gpio_global();
	a375_cascade_irq_gpio_private();

	a375_cascade_irq_msi_global();
	a375_cascade_irq_msi_private();

	a375_cascade_irq_errors();
}
