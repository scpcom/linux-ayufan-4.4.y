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
#include "cpu/mvCpu.h"

#define SOC_PPI_CAUSE			0x21080
#define SOC_PPI_MASK_SET		0x210b8
#define SOC_PPI_MASK_CLEAR		0x210bc
#define SOC_PPI_CPU_REG(cpu, reg)	((reg) + ((cpu) ? 0x900 : 0x800))
#define SOC_PPI_CAUSE_BITS		0x1fffffff

static DEFINE_RAW_SPINLOCK(irq_controller_lock);

/*
 * Global GPIO interrupt handling
 */
static void a38x_gpio_irq_mask(struct irq_data *d)
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

static void a38x_gpio_irq_unmask(struct irq_data *d)
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

static struct irq_chip a38x_irq_chip = {
	.name = "a38x_gpio_irq",
	.irq_mask = a38x_gpio_irq_mask,
	.irq_mask_ack = a38x_gpio_irq_mask,
	.irq_unmask = a38x_gpio_irq_unmask,
	.irq_disable = a38x_gpio_irq_mask,
	.irq_enable = a38x_gpio_irq_unmask,
};

/*
 * gpio_cascade_irq is in [85:88] or [90:94]
 */
static void a38x_gpio_cascade_irq_handler(unsigned int gpio_cascade_irq,
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

static void __init a38x_cascade_irq_gpio_global(void)
{
	int irq, irq_base;

	irq_base = irq_alloc_descs(-1, IRQ_START_GLOBAL_GPIO,
				   NR_IRQS_GLOBAL_GPIO, 0);
	if (IS_ERR_VALUE(irq_base))
		BUG();

	for (irq = IRQ_START_GLOBAL_GPIO;
	     irq < IRQ_START_GLOBAL_GPIO + NR_IRQS_GLOBAL_GPIO; irq++) {
		irq_set_chip_and_handler(irq, &a38x_irq_chip, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	irq_set_chained_handler(IRQ_GLOBAL_GPIO_0_7,
				a38x_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_8_15,
				a38x_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_16_23,
				a38x_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_24_31,
				a38x_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_32_39,
				a38x_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_40_47,
				a38x_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_48_55,
				a38x_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_56_63,
				a38x_gpio_cascade_irq_handler);
	irq_set_chained_handler(IRQ_GLOBAL_GPIO_64_66,
				a38x_gpio_cascade_irq_handler);
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

#ifdef CONFIG_SMP
static int mpic_irq_set_affinity(struct irq_data *d,
		const struct cpumask *mask_val, bool force)
{
	struct mpic_data *mpic = irq_get_chip_data(d->irq);
	unsigned long irq = mpic_hw_irq(mpic, d->irq);
	int cpu, count = 0;

	raw_spin_lock(&irq_controller_lock);
	for_each_online_cpu(cpu)
		/*
		 * Forbid multicore interrupt affinity.
		 * This is required since the MPIC HW doesn't limit
		 * several CPUs from acknowledging the same interrupt.
		 */
		if ((count == 0) && cpumask_test_cpu(cpu, mask_val)) {
			writel_relaxed(irq, SOC_PPI_CPU_REG(cpu, mpic->regs.unmask));
			count++;
		} else
			writel_relaxed(irq, SOC_PPI_CPU_REG(cpu, mpic->regs.mask));
	raw_spin_unlock(&irq_controller_lock);

	return IRQ_SET_MASK_OK;
}
#endif

static struct irq_chip mpic_chip = {
	.name = "a38x_mpic",
	.irq_mask = mpic_irq_mask,
	.irq_unmask = mpic_irq_unmask,
#ifdef CONFIG_SMP
	.irq_set_affinity = mpic_irq_set_affinity,
#endif
};

static void mpic_handle_cascade_irq(unsigned int irq, struct irq_desc *desc)
{
	struct mpic_data *mpic = &mpic_data;
	struct irq_chip *chip = irq_get_chip(irq);
	unsigned long cause;
#ifdef CONFIG_SMP
	struct irq_data *irqd;
#endif

	chained_irq_enter(chip, desc);

	raw_spin_lock(&irq_controller_lock);
	cause = readl_relaxed(mpic->regs.cause);
	raw_spin_unlock(&irq_controller_lock);

	/* leave only relevant cause bits */
	cause &= SOC_PPI_CAUSE_BITS;
	if (cause == 0)
		goto out;	/* spurious irq ? */

	while (cause) {
		irq = ffs(cause) - 1;
		cause &= ~(1 << irq);
#ifdef CONFIG_SMP
		irqd = irq_get_irq_data(mpic->irq_base + irq);
		if (!cpumask_test_cpu(whoAmI(), irqd->affinity))
			continue;
#endif
		generic_handle_irq(mpic->irq_base + irq);
	}

out:
	chained_irq_exit(chip, desc);
}

static void __init a38x_cascade_irq_mpic(int chained_irq, int irq_start, int nr_irqs)
{
	int irq, cpu;
	struct mpic_data *mpic = &mpic_data;

	mpic->irq_base = irq_alloc_descs(-1, irq_start, nr_irqs, 0);
	if (IS_ERR_VALUE(mpic->irq_base))
		BUG();

	for (irq = irq_start; irq < irq_start + nr_irqs; irq++) {
		irq_set_status_flags(irq, IRQ_LEVEL);
		irq_set_chip_and_handler(irq, &mpic_chip, handle_level_irq);
		irq_set_chip_data(irq, &mpic_data);
		set_irq_flags(irq, IRQF_VALID);

		/* Mask SOC PPI interrupts on all cores */
		for_each_online_cpu(cpu)
			writel_relaxed(mpic_hw_irq(mpic, irq),
					SOC_PPI_CPU_REG(cpu, mpic->regs.mask));
	}

	irq_set_chained_handler(chained_irq, mpic_handle_cascade_irq);
}

static void __init a38x_cascade_irq_gpio_private(void)
{
	/* TBD */
}

static void __init a38x_cascade_irq_msi_global(void)
{
	/* TBD */
}

static void __init a38x_cascade_irq_msi_private(void)
{
	/* TBD */
}

static void __init a38x_cascade_irq_errors(void)
{
	/* TBD */
}

/*
 * Init GIC and MPIC and setup cascade irq
 * handling for GPIO, MSI and Error interrupts.
 */
void __init a38x_init_irq(void)
{
	gic_init(0, 29,
		 (void __iomem *)(INTER_REGS_VIRT_BASE + A9_MPCORE_GIC_DIST),
		 (void __iomem *)(INTER_REGS_VIRT_BASE + A9_MPCORE_GIC_CPU));

	a38x_cascade_irq_mpic(IRQ_PRIV_MPIC_PPI_IRQ, IRQ_START_PRIV_SOC_PPI, NR_IRQS_PRIV_SOC_PPI);

	a38x_cascade_irq_gpio_global();
	a38x_cascade_irq_gpio_private();

	a38x_cascade_irq_msi_global();
	a38x_cascade_irq_msi_private();

	a38x_cascade_irq_errors();
}
