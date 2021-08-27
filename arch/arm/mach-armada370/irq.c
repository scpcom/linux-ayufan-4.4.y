/*
 * arch/arm/mach/irq.c
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <asm/mach/arch.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"
#include "mvOs.h"
#include "ctrlEnv/sys/mvCpuIfRegs.h"

unsigned int  irq_int_type[NR_IRQS];

static void axp_unmask_fabric_interrupt(void)
{
	u32 val;

	val = MV_REG_READ(CPU_CF_LOCAL_MASK_REG);
	val |= 1;
	MV_REG_WRITE(CPU_CF_LOCAL_MASK_REG, val);
}

static void axp_mask_fabric_interrupt(void)
{
	u32 val;

	val = MV_REG_READ(CPU_CF_LOCAL_MASK_REG);
	val &=  ~1;
	MV_REG_WRITE(CPU_CF_LOCAL_MASK_REG, val);
}

void axp_irq_mask(struct irq_data *d)
{

	u32 irq=d->irq;

	if (irq < IRQ_MAIN_INTS_NUM) {
		MV_REG_BIT_RESET(CPU_INT_SOURCE_CONTROL_REG(irq), BIT0);

	}
	else if (irq < (IRQ_AURORA_GPIO_START + NR_GPIO_IRQS)) {
		MV_U32 bitmask = 1 << (irq & 0x1F);
		MV_U32 reg = (irq - IRQ_AURORA_GPIO_START) >> 5;
		MV_REG_BIT_RESET(GPP_INT_LVL_REG(reg), bitmask);
	}
	else if (irq < (IRQ_AURORA_ERR_START + NR_SOC_MAIN_ERR_IRQS)) {
		MV_U32 bitmask = (1 << (irq - IRQ_AURORA_ERR_START));
		MV_REG_BIT_RESET(MV_SOC_MAIN_INT_ERR_MASK_REG, bitmask);
	}
	else
		printk("%s: Error, invalid irqnr(%u)\n", __func__, irq);
}

void axp_irq_unmask(struct irq_data *d)
{
        u32 irq=d->irq;
	if (irq < IRQ_MAIN_INTS_NUM) {
		if(irq < 16)
			MV_REG_BIT_SET(CPU_INT_SOURCE_CONTROL_REG(irq), BIT0);
		else
			MV_REG_BIT_SET(CPU_INT_SOURCE_CONTROL_REG(irq), (BIT0 | BIT28));
	}
	else if (irq < (IRQ_AURORA_GPIO_START + NR_GPIO_IRQS)) {
		MV_U32 bitmask = 1 << (irq & 0x1F);
		MV_U32 reg = (irq - IRQ_AURORA_GPIO_START) >> 5;
		MV_REG_BIT_SET(GPP_INT_LVL_REG(reg), bitmask);
	}
	else if (irq < (IRQ_AURORA_ERR_START + NR_SOC_MAIN_ERR_IRQS)) {
		MV_U32 bitmask = (1 << (irq - IRQ_AURORA_ERR_START));
		MV_REG_BIT_SET(MV_SOC_MAIN_INT_ERR_MASK_REG, bitmask);
	}
	else
		printk("%s: Error, invalid irqnr(%u)\n", __func__, irq);
}

static struct irq_chip axp_irq_chip = {
	.name		= "armada370_irq",
	.irq_mask	= axp_irq_mask,
	.irq_mask_ack	= axp_irq_mask,
	.irq_unmask	= axp_irq_unmask,
	.irq_disable	= axp_irq_mask,
	.irq_enable	= axp_irq_unmask,
};

void __init axp_init_irq(void)
{
	u32 irq, i;

	/* MASK all interrupts */
	for (irq = 0; irq < IRQ_MAIN_INTS_NUM; irq++) {
		axp_irq_mask(irq_get_irq_data(irq));
	}

	/* Clear SoC main error masks & cause registers */
	MV_REG_WRITE(MV_SOC_MAIN_INT_ERR_MASK_REG, 0);
	MV_REG_WRITE(MV_SOC_MAIN_INT_ERR_CAUSE_REG, 0);

	/* Enable SoC main error summary bit */
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_SOC_ERROR));

	/* Disable and clear all GPIO interrupts */
        for(i = 0; i < MV_GPP_MAX_GROUP; i++) {
                MV_REG_WRITE(GPP_INT_MASK_REG(i), 0x0);
		MV_REG_WRITE(GPP_INT_LVL_REG(i), 0x0);
		MV_REG_WRITE(GPP_INT_CAUSE_REG(i), 0x0);
        }

	/* Init GPP IRQs in default level mode */
	for (i = 0; i < NR_IRQS; i++)
		irq_int_type[i] = GPP_IRQ_TYPE_LEVEL;

	/* Enable GPIO interrupts */
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_0_7));
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_8_15));
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_16_23));
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_24_31));
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_32_39));
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_40_47));
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_48_55));
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_56_63));
	axp_irq_unmask(irq_get_irq_data(IRQ_AURORA_GPIO_64_66));

	/* Register IRQ sources */
	for (irq = 0; irq < NR_IRQS; irq++) {
		irq_set_chip(irq, &axp_irq_chip);
		irq_set_chip_data(irq, 0);
		irq_set_handler(irq, handle_level_irq);
		irq_set_status_flags(irq,IRQ_LEVEL);
		set_irq_flags(irq, IRQF_VALID);
	}
}

int pmu_request_irq(int irq, irq_handler_t handler)
{
	int ret = request_irq(irq, handler, IRQF_DISABLED | IRQF_NOBALANCING, "armpmu", NULL);
	if (!ret)
		axp_unmask_fabric_interrupt();

	return ret;
}

void pmu_free_irq(int irq)
{
	axp_mask_fabric_interrupt();
	free_irq(irq, NULL);
}
