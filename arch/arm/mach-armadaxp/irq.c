/*
* arch/arm/mach/irq.c
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <plat/msi.h>
#include <mach/armadaxp.h>
#include "include/mach/smp.h"
#include "mvOs.h"
#include "mvSysGppConfig.h"

/* Initialize gpiolib. */
void __init orion_gpio_init(int gpio_base, int ngpio,
			    u32 base, int mask_offset, int secondary_irq_base);
/*
 * GPIO interrupt handling.
 */
void orion_gpio_irq_handler(int irqoff);

unsigned int  irq_int_type[NR_IRQS];
#define ENABLED_DOORBELS 	(0xF0FF)

#if (defined(CONFIG_PERF_EVENTS) && defined(CONFIG_HW_PERF_EVENTS)) || defined(CONFIG_ERROR_HANDLING)
static void axp_unmask_fabric_interrupt(int cpu)
{
	u32 val;
	val = MV_REG_READ(CPU_CF_LOCAL_MASK_REG(cpu));
	val |=  (1 << cpu);
	MV_REG_WRITE(CPU_CF_LOCAL_MASK_REG(cpu), val);

#ifdef CONFIG_SMP
	if (cpu > 0) { /*enabled for both cpu */
		val = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_MP));
		/* FIXME: assuming all 4 cpus */
		val |= 0xf;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_MP), val);
	}
#endif
}

static void axp_mask_fabric_interrupt(int cpu)
{
	u32 val;
	val = MV_REG_READ(CPU_CF_LOCAL_MASK_REG(cpu));
	val &=  ~(1 << cpu);
	MV_REG_WRITE(CPU_CF_LOCAL_MASK_REG(cpu), val);

#ifdef CONFIG_SMP
	if (cpu > 0) { /*disabled for both cpu */
		val = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_MP));
		/* FIXME: assuming all 4 cpus */
		val &= ~0xf;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_MP), val);
}
#endif
}
#endif /* (CONFIG_PERF_EVENTS && CONFIG_HW_PERF_EVENTS) || CONFIG_ERROR_HANDLING */

void axp_irq_mask(struct irq_data *d)
{	
	int cpu;
	/* In case of shared IRQ - Disable the IRQ and manually mask each of the CPUs */
	/* In case of Network Per CPU IRQ and SMP - Mask all CPUs */
	/* In case of Per CPU IRQ - Mask the only the requesting CPU */
	if (d->irq > MAX_PER_CPU_IRQ_NUMBER)
		MV_REG_WRITE(CPU_INT_CLEAR_ENABLE_REG, d->irq);
#ifdef CONFIG_SMP
	else if ((d->irq >= IRQ_AURORA_GBE0_FIC) &&
			(d->irq <= IRQ_AURORA_GBE3_SIC)) {
		for_each_possible_cpu(cpu) {
			MV_REG_WRITE(CPU_INT_SET_MASK_REG(get_hw_cpu_id(cpu)), d->irq);
		}
	} else
		MV_REG_WRITE(CPU_INT_SET_MASK_LOCAL_REG, d->irq);
#else
	MV_REG_WRITE(CPU_INT_SET_MASK_LOCAL_REG, d->irq);
#endif

#if (defined(CONFIG_PERF_EVENTS) && defined(CONFIG_HW_PERF_EVENTS))	\
			|| defined(CONFIG_ERROR_HANDLING)
	if(d->irq == IRQ_AURORA_MP){
		for_each_online_cpu(cpu) {
			axp_mask_fabric_interrupt(cpu);
		}
	}
#endif
}

void axp_irq_unmask(struct irq_data *d)
{
	int cpu;

	/* In case of shared IRQ - Enable the IRQ */
	/* In case of Network Per CPU IRQ and SMP - Set correct affinity to the IRQ */
	/* In case of Per CPU IRQ - UnMask the only the requesting CPU */
	if (d->irq > MAX_PER_CPU_IRQ_NUMBER)
		MV_REG_WRITE(CPU_INT_SET_ENABLE_REG, d->irq);
#ifdef CONFIG_SMP
	else if ((d->irq >= IRQ_AURORA_GBE0_FIC) && (d->irq <= IRQ_AURORA_GBE3_SIC)) {
		for_each_cpu(cpu, d->affinity)
			MV_REG_WRITE(CPU_INT_CLEAR_MASK_REG(get_hw_cpu_id(cpu)), d->irq);
	} else
		MV_REG_WRITE(CPU_INT_CLEAR_MASK_LOCAL_REG, d->irq);
#else
	MV_REG_WRITE(CPU_INT_CLEAR_MASK_LOCAL_REG, d->irq);
#endif

#if (defined(CONFIG_PERF_EVENTS) && defined(CONFIG_HW_PERF_EVENTS)) || defined(CONFIG_ERROR_HANDLING)
	if(d->irq == IRQ_AURORA_MP){
		for_each_online_cpu(cpu) {
			axp_unmask_fabric_interrupt(cpu);
		}
	}
#endif
}

#ifdef CONFIG_SMP
int axp_set_affinity(struct irq_data *d, const struct cpumask *mask_val,bool force)
{
	int cpu;
	struct cpumask mask_temp;

	cpumask_and(&mask_temp, mask_val, cpu_online_mask);
	cpumask_copy(d->affinity, &mask_temp);
	d->node = cpumask_first(&mask_temp);

	for_each_possible_cpu(cpu) {
		if (*cpus_addr(mask_temp) & (1 << cpu))
			MV_REG_WRITE(CPU_INT_CLEAR_MASK_REG(get_hw_cpu_id(cpu)), d->irq);
		else
			MV_REG_WRITE(CPU_INT_SET_MASK_REG(get_hw_cpu_id(cpu)), d->irq);
	}

	return 0;
}
#endif

#ifdef CONFIG_SMP
void axp_ipi_init(void)
{
	struct irq_data *d = irq_get_irq_data(IRQ_AURORA_IN_DRBL_LOW);
	unsigned long temp;
	
	/* open IPI mask */
	temp = MV_REG_READ(AXP_IN_DRBEL_MSK) | ENABLED_DOORBELS;
	MV_REG_WRITE(AXP_IN_DRBEL_MSK, temp);

	axp_irq_unmask(d);
}
#endif

static void gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	BUG_ON(irq < IRQ_AURORA_GPIO_0_7 || irq > IRQ_AURORA_GPIO_56_63);

	orion_gpio_irq_handler((irq - IRQ_AURORA_GPIO_0_7) << 3);
}

void axp_gpio_init(void)
{
	u32 irq;

	orion_gpio_init(0, 32, INTER_REGS_VIRT_BASE + MV_GPP_REGS_BASE(0), 0,
			 IRQ_AURORA_GPIO_START);
	orion_gpio_init(32, 32, INTER_REGS_VIRT_BASE + MV_GPP_REGS_BASE(1), 0,
			 IRQ_AURORA_GPIO_START + 32);
	orion_gpio_init(64, 3, INTER_REGS_VIRT_BASE + MV_GPP_REGS_BASE(2), 0,
			 IRQ_AURORA_GPIO_START + 64);

	for (irq = IRQ_AURORA_GPIO_0_7; irq <= IRQ_AURORA_GPIO_64_66; irq++) {
		if (irq == 86)
			continue;
		/* Enable GPIO interrupts */
		axp_irq_unmask(irq_get_irq_data(irq));
		/* Initialize gpiolib for GPIOs 0-64 */
		irq_set_chained_handler(irq, gpio_irq_handler);
#ifdef CONFIG_SMP
		/* Set affinity */
		axp_set_affinity(irq_get_irq_data(irq), cpumask_of(0), 0);
#endif
	}
}

static struct irq_chip axp_irq_chip = {
	.name		= "axp_irq",
	.irq_mask	= axp_irq_mask,
	.irq_mask_ack	= axp_irq_mask,
	.irq_unmask	= axp_irq_unmask,
#ifdef CONFIG_SMP
	.irq_set_affinity   = axp_set_affinity,
#endif
};

#ifdef CONFIG_CPU_PM
#ifdef CONFIG_SMP
/*
 * In case of SMP configuration, the axp_irq_unmask will just enable
 * the global IRQ and leave it masked. Therefore, when retruning from
 * suspend to RAM, We must manually unmask the global IRQs.
 */
void axp_irq_restore(void)
{
	int irq, cpu;
	struct irq_data *d;
	struct cpumask mask_temp;

	/* Unmask global IRQs according to the IRQ structure affinity value */
	for (irq = MAX_PER_CPU_IRQ_NUMBER; irq < IRQ_MAIN_INTS_NUM; irq++) {
		d = irq_get_irq_data(irq);
		cpumask_and(&mask_temp, d->affinity, cpu_online_mask);

		for_each_possible_cpu(cpu) {
			if (*cpus_addr(mask_temp) & (1 << cpu))
				MV_REG_WRITE(CPU_INT_CLEAR_MASK_REG(get_hw_cpu_id(cpu)), d->irq);
			else
				MV_REG_WRITE(CPU_INT_SET_MASK_REG(get_hw_cpu_id(cpu)), d->irq);
		}
	}
}
#else
void axp_irq_restore(void)
{
}
#endif
#endif

void __init axp_init_irq(void)
{
	u32 irq;
	/* MASK all interrupts */
	for (irq = 0; irq < IRQ_MAIN_INTS_NUM; irq++)
#ifndef CONFIG_MV_AMP_ENABLE
		axp_irq_mask(irq_get_irq_data(irq));
#endif
	/* Register IRQ sources */
	for (irq = 0; irq < IRQ_AURORA_MSI_START ; irq++) {
		irq_set_chip(irq, &axp_irq_chip);
		irq_set_chip_data(irq, 0);
		irq_set_handler(irq, handle_level_irq);
		irq_set_status_flags(irq,IRQ_LEVEL);
#ifdef CONFIG_SMP
	/* Network Per CPU IRQ are treated as shared IRQs */
		if ((irq < MAX_PER_CPU_IRQ_NUMBER) && (irq != IRQ_AURORA_MP) &&
			((irq < IRQ_AURORA_GBE0_FIC) || (irq > IRQ_AURORA_GBE3_SIC))) {
			irq_set_chip_and_handler(irq, &axp_irq_chip,
				handle_percpu_devid_irq);
			irq_set_percpu_devid(irq);
		}
#endif
		set_irq_flags(irq, IRQF_VALID);
	}

#ifdef CONFIG_SMP
	/* Set the default affinity to the boot cpu. */
	cpumask_clear(irq_default_affinity);
	cpumask_set_cpu(smp_processor_id(), irq_default_affinity);

	axp_ipi_init();
#endif
	axp_gpio_init();
	armada_msi_init();
}

#if (defined(CONFIG_PERF_EVENTS) && defined(CONFIG_HW_PERF_EVENTS))
/*
 * This functions used by Pixiu driver under tools/pixiu.
 */
int pmu_request_irq(int irq, irq_handler_t handler)
{
	int i;
	int ret = request_irq(irq, handler, IRQF_DISABLED |
			IRQF_NOBALANCING, "armpmu", NULL);
	if (!ret) {
		for_each_online_cpu(i) {
			axp_unmask_fabric_interrupt(i);
		}
	}
	return ret;
}
EXPORT_SYMBOL(pmu_request_irq);

void pmu_free_irq(int irq)
{
	int i;
	for_each_online_cpu(i) {
		axp_mask_fabric_interrupt(i);
	}
	free_irq(irq, NULL);
}

EXPORT_SYMBOL(pmu_free_irq);
#endif
