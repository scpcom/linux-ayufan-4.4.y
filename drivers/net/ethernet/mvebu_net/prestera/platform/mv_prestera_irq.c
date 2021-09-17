#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvOs.h"
#include "mv_prestera_irq.h"
#include "mv_prestera.h"
#include "mv_prestera_pci.h"

#ifdef MV_PP_DBG
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif

#define PRESTERA_MAX_INTERRUPTS 4
#define SEMA_DEF_VAL		0

#define IRQ_AURORA_SW_CORES	(IRQ_AURORA_SW_CORE0 | IRQ_AURORA_SW_CORE1 | IRQ_AURORA_SW_CORE2)
#define IRQ_SWITCH_MASK		(0x7 << (IRQ_AURORA_SW_CORE0 - (CPU_INT_SOURCE_CONTROL_IRQ_OFFS + 1)))

#define SW_CORES		0x3

static struct pp_dev	*assigned_irq[PRESTERA_MAX_INTERRUPTS];
static int		assinged_irq_nr;
static int		short_bh_count;

int prestera_int_bh_cnt_get(void)
{
	return short_bh_count;
}

static void mv_ac3_bc2_enable_switch_irq(uintptr_t i_regs, bool enable)
{
	int reg_val, i;
	unsigned int addr;

	for (i = 0; i < SW_CORES; i++) {

		addr = CPU_INT_SOURCE_CONTROL_REG((IRQ_AURORA_SW_CORE0 + i));
		reg_val = readl(i_regs + addr);
		dprintk("irq status and ctrl(0x%x) = 0x%x\n", addr, reg_val);

		if (enable)
			reg_val |= (PEX_IRQ_EN) | (PEX_IRQ_EP);
		else
			reg_val &= ~(PEX_IRQ_EN) & ~(PEX_IRQ_EP);

		writel(reg_val, i_regs + addr);

		dprintk("irq status and ctrl(0x%x) = 0x%x\n",
			addr, readl(i_regs + addr));

		writel(IRQ_AURORA_SW_CORE0 + i,
			i_regs + CPU_INT_CLEAR_MASK_LOCAL_REG);
		dprintk("clr reg(0x%x)\n", CPU_INT_CLEAR_MASK_LOCAL_REG);
	}
}

static irqreturn_t prestera_tl_isr(int		irq,
					void		*dev_id)
{
	struct pp_dev *ppdev = dev_id;

	disable_irq_nosync(irq);

	tasklet_hi_schedule((struct tasklet_struct *)ppdev->irq_data.tasklet);

	short_bh_count++;

	return IRQ_HANDLED;
}

static irqreturn_t prestera_tl_isr_pci(int irq, void *dev_id)
{
	struct pp_dev *ppdev = dev_id;
	int reg;

	reg = readl(ppdev->config.base + MSYS_CAUSE_VEC1_REG_OFFS);
	dprintk("msys cause reg 0x%x, switch_mask 0x%x\n",
							 reg, IRQ_SWITCH_MASK);

	if ((reg & IRQ_SWITCH_MASK) == 0)
		return IRQ_NONE;

	disable_irq_nosync(irq);

	tasklet_hi_schedule((struct tasklet_struct *)ppdev->irq_data.tasklet);

	short_bh_count++;

	return IRQ_HANDLED;
}

static void prestera_bh(unsigned long data)
{
	 
	up(&((struct intData *)data)->sem);
}

int prestera_int_connect(struct pp_dev		*ppdev,
			 void			*routine,
			 struct intData		**cookie)
{
	unsigned int		status, intVec = ppdev->irq_data.intVec;
	struct intData		*irq_data = &ppdev->irq_data;
	struct tasklet_struct	*tasklet;
#if defined(MY_ABC_HERE)
	unsigned long       flags;
#endif  

	tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
	if (NULL == tasklet) {
		printk(KERN_ERR "kmalloc failed\n");
		return -ENOMEM;
	}

	*cookie = irq_data;

	sema_init(&(irq_data->sem), SEMA_DEF_VAL);

	irq_data->tasklet = tasklet;

	tasklet_init(tasklet, prestera_bh, (unsigned long)irq_data);

	if (((ppdev->devId & ~MV_DEV_FLAVOUR_MASK) == MV_BOBCAT2_DEV_ID ||
		 (ppdev->devId & ~MV_DEV_FLAVOUR_MASK) == MV_ALLEYCAT3_DEV_ID) &&
								   ppdev->on_pci_bus == 1)
		status = request_irq(intVec, prestera_tl_isr_pci, IRQF_SHARED,
							  "mvPP", (void *)ppdev);
	else
		status = request_irq(intVec, prestera_tl_isr, IRQF_DISABLED,
							  "mvPP", (void *)ppdev);

	if (status) {
		panic("Can not assign IRQ %d to PresteraDev\n", intVec);
		return -1;
	}

	printk(KERN_DEBUG "%s: connected Prestera IRQ - %d\n", __func__, intVec);
	disable_irq_nosync(intVec);
#if defined(MY_ABC_HERE)
	local_irq_save(flags);
#else  
	local_irq_disable();
#endif  
	if (assinged_irq_nr < PRESTERA_MAX_INTERRUPTS) {
		assigned_irq[assinged_irq_nr++] = ppdev;
#if defined(MY_ABC_HERE)
		local_irq_restore(flags);
#else  
		local_irq_enable();
#endif  
	} else {
#if defined(MY_ABC_HERE)
		local_irq_restore(flags);
#else  
		local_irq_enable();
#endif  
		printk(KERN_DEBUG "%s: too many irqs assigned\n", __func__);
	}

	if (((ppdev->devId & ~MV_DEV_FLAVOUR_MASK) == MV_BOBCAT2_DEV_ID ||
		(ppdev->devId & ~MV_DEV_FLAVOUR_MASK) == MV_ALLEYCAT3_DEV_ID) && ppdev->on_pci_bus == 1)
		mv_ac3_bc2_enable_switch_irq(ppdev->config.base, true);

	return 0;
}

void prestera_int_init(void)
{
	assinged_irq_nr = 0;
	short_bh_count = 0;
}

int prestera_int_cleanup(void)
{
	struct intData *irq_data;
	struct pp_dev *ppdev;

	while (assinged_irq_nr > 0) {

		ppdev = assigned_irq[--assinged_irq_nr];
		irq_data = &ppdev->irq_data;

		if (((ppdev->devId & ~MV_DEV_FLAVOUR_MASK) == MV_BOBCAT2_DEV_ID ||
			 (ppdev->devId & ~MV_DEV_FLAVOUR_MASK) == MV_ALLEYCAT3_DEV_ID) &&
									  ppdev->on_pci_bus == 1)
			mv_ac3_bc2_enable_switch_irq(ppdev->config.base, false);
		else
			disable_irq_nosync(irq_data->intVec);

		free_irq(irq_data->intVec, (void *)ppdev);
		tasklet_kill(irq_data->tasklet);
		kfree(irq_data->tasklet);
	}
	return 0;
}
