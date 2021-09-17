#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel_stat.h>
#include <linux/irq.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/seq_file.h>
#include <linux/ratelimit.h>
#if defined (MY_DEF_HERE)
#include <linux/export.h>
#endif  

unsigned long irq_err_count;

int arch_show_interrupts(struct seq_file *p, int prec)
{
#ifdef CONFIG_SMP
	show_ipi_list(p, prec);
#endif
	seq_printf(p, "%*s: %10lu\n", prec, "Err", irq_err_count);
	return 0;
}

void handle_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();

	if (unlikely(irq >= nr_irqs)) {
		pr_warn_ratelimited("Bad IRQ%u\n", irq);
		ack_bad_irq(irq);
	} else {
		generic_handle_irq(irq);
	}

	irq_exit();
	set_irq_regs(old_regs);
}

void __init set_handle_irq(void (*handle_irq)(struct pt_regs *))
{
	if (handle_arch_irq)
		return;

	handle_arch_irq = handle_irq;
}

void __init init_IRQ(void)
{
	irqchip_init();
	if (!handle_arch_irq)
		panic("No interrupt controller found.");
}

#if defined (MY_DEF_HERE)
void set_irq_flags(unsigned int irq, unsigned int flags)
{
	unsigned long clr = 0;
	unsigned long set = IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;

	if (irq >= nr_irqs) {
		pr_err("Trying to set irq flags for IRQ%d\n", irq);
		return;
	}

	if (flags & IRQF_VALID)
		clr |= IRQ_NOREQUEST;
	if (flags & IRQF_PROBE)
		clr |= IRQ_NOPROBE;
	if (!(flags & IRQF_NOAUTOEN))
		clr |= IRQ_NOAUTOEN;
	 
	irq_modify_status(irq, clr, set & ~clr);
}
EXPORT_SYMBOL_GPL(set_irq_flags);
#endif  
