/*
 * arch/arm/plat-armada/msi.c
 *
 * Marvell Armada SoC MSI,MSI-X handling.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/pci.h>
#include <linux/msi.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include "mvOs.h"

static DECLARE_BITMAP(msi_irq_in_use, NR_PRIVATE_MSI_IRQS);

void armada_msi_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	int j;
	unsigned long status;

	/* Read Inbound Shared Doorbell registers and find any active interrupts,
	* then call ISR for each active interrupt
	*/
	status = MV_REG_READ(AXP_IN_DRBEL_CAUSE) & 0xFFFF0000;
	if (!status)
		return;

	j = find_first_bit(&status, 32);
	
	MV_REG_WRITE(AXP_IN_DRBEL_CAUSE, ~(1 << j));
	status = MV_REG_READ(AXP_IN_DRBEL_CAUSE);
	/* write back to clear bit */
	generic_handle_irq(IRQ_AURORA_MSI_START + j - NR_PRIVATE_MSI_IRQS);
}

void armada_msi_init_unmask(void)
{
	unsigned long temp;

	/* Unmask private doorbells 16-31 */
	temp = MV_REG_READ(AXP_IN_DRBEL_MSK) | (0xFFFF0000);
	MV_REG_WRITE(AXP_IN_DRBEL_MSK, temp);

#ifdef CONFIG_SMP
	/* Unmask doorbell high IRQ for all CPUs */
	temp = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_HIGH));
	temp |= 0xf;
	MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_HIGH), temp);
#endif
}

void __init armada_msi_init(void)
{
	irq_set_chained_handler(IRQ_AURORA_IN_DRBL_HIGH, armada_msi_irq_handler);

	armada_msi_init_unmask();
}

/*
 * Dynamic irq allocate and deallocation
 */
int create_irq(void)
{
	int irq, pos;

again:
	pos = find_first_zero_bit(msi_irq_in_use, NR_PRIVATE_MSI_IRQS);
	irq = IRQ_AURORA_MSI_START + pos;
	if (irq > NR_IRQS)
		return -ENOSPC;
	/* test_and_set_bit operates on 32-bits at a time */
	if (test_and_set_bit(pos, msi_irq_in_use))
		goto again;

	dynamic_irq_init(irq);
	irq_clear_status_flags(irq, IRQ_NOREQUEST);
	
	return irq;
}

void destroy_irq(unsigned int irq)
{
	int pos = irq - IRQ_AURORA_MSI_START;

	dynamic_irq_cleanup(irq);

	clear_bit(pos, msi_irq_in_use);
}

void arch_teardown_msi_irq(unsigned int irq)
{
	destroy_irq(irq);
}

static void armada_msi_nop(struct irq_data *d)
{
	return;
}

#ifdef CONFIG_SMP
int armada_msi_set_affinity(struct irq_data *d, const struct cpumask *mask_val,bool force)
{
	struct msi_msg msg;
	u32 irq=d->irq;
	int msi_irq;
	int i;
	
	msi_irq = irq - IRQ_AURORA_MSI_START;
	msg.address_hi = 0x0;
	msg.address_lo = AXP_SW_TRIG_IRQ_PHYS;
	msg.data = ((msi_irq + NR_PRIVATE_MSI_GROUP) & AXP_SW_TRIG_IRQ_INITID_MASK);

	for_each_cpu(i, mask_val)
		msg.data |= (0x1 << (AXP_SW_TRIG_IRQ_CPU_TARGET_OFFS + i));

	write_msi_msg(irq, &msg);
	cpumask_copy(d->affinity, mask_val);
	
	return 0;
}

void second_cpu_msi_init(void)
{
	unsigned long temp;
	/* Unmask private doorbells 16-31 */
	temp = MV_REG_READ(AXP_IN_DRBEL_MSK) | (0xFFFF0000);
	MV_REG_WRITE(AXP_IN_DRBEL_MSK, temp);

#ifdef CONFIG_SMP
	/* Unmask doorbell high IRQ for all CPUs */
	temp = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_HIGH));
	temp |= 0xf;
	MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_HIGH), temp);
#endif
}
#endif

struct irq_chip armada_msi_irq_chip = {
	.name			= "axp_msi_irq",
	.irq_ack 		= armada_msi_nop,
	.irq_enable 		= unmask_msi_irq,
	.irq_disable 		= mask_msi_irq,
	.irq_mask 		= mask_msi_irq,
	.irq_unmask 		= unmask_msi_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	= armada_msi_set_affinity,
#endif
};

int arch_setup_msi_irq(struct pci_dev *pdev, struct msi_desc *desc)
{
	int irq = create_irq();
	int msi_irq;
	struct msi_msg msg;

	if (irq < 0)
		return irq;

	msi_irq = irq - IRQ_AURORA_MSI_START;
	irq_set_msi_desc(irq, desc);

	msg.address_hi = 0x0;
	msg.address_lo = AXP_SW_TRIG_IRQ_PHYS;
	msg.data = (0x1 << AXP_SW_TRIG_IRQ_CPU_TARGET_OFFS) | 
			((msi_irq + NR_PRIVATE_MSI_GROUP) & AXP_SW_TRIG_IRQ_INITID_MASK);

	write_msi_msg(irq, &msg);
	irq_set_chip_and_handler(irq, &armada_msi_irq_chip, handle_edge_irq);

	return 0;
}
