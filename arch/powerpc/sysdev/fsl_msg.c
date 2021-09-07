#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef MY_DEF_HERE
 
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <asm/fsl_msg.h>
#include <asm/mpic.h>

struct msg_addr {
	u32 addr;
	u8 res[12];
};

struct fsl_msg_regs {
	struct msg_addr unit[FSL_NUM_MPIC_MSGS];
	u8 	res0[192];
	u32	mer;
	u8	res1[12];
	u32	msr;
};

struct fsl_msg {
	struct fsl_msg_regs __iomem *regs;
	struct fsl_msg_unit messages[FSL_NUM_MPIC_MSGS];
	spinlock_t lock;
	struct list_head list_node;
};

static LIST_HEAD(fsl_msgs);

struct fsl_msg_unit *fsl_get_msg_unit(void)
{
	struct fsl_msg *fsl_msg;
	unsigned long flags;
	int i;

	list_for_each_entry(fsl_msg, &fsl_msgs, list_node) {
		spin_lock_irqsave(&fsl_msg->lock, flags);

		for (i = 0; i < ARRAY_SIZE(fsl_msg->messages); i++) {
			if (!fsl_msg->messages[i].requested) {
				fsl_msg->messages[i].requested = true;
				spin_unlock_irqrestore(&fsl_msg->lock, flags);
				return &fsl_msg->messages[i];
			}
		}

		spin_unlock_irqrestore(&fsl_msg->lock, flags);
	}

	if (fsl_msg)
		return ERR_PTR(-EBUSY);

	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(fsl_get_msg_unit);

void fsl_release_msg_unit(struct fsl_msg_unit *msg)
{
	struct fsl_msg *fsl_msg = msg->fsl_msg;
	unsigned long flags;

	spin_lock_irqsave(&fsl_msg->lock, flags);
	msg->requested = false;
	spin_unlock_irqrestore(&fsl_msg->lock, flags);
}
EXPORT_SYMBOL(fsl_release_msg_unit);

void fsl_clear_msg(struct fsl_msg_unit *msg)
{
	u32 tmp32;

	fsl_read_msg(msg, &tmp32);
}
EXPORT_SYMBOL(fsl_clear_msg);

void fsl_enable_msg(struct fsl_msg_unit *msg)
{
	u32 tmp32;

	tmp32 = in_be32(msg->mer);
	out_be32(msg->mer, tmp32 | (1 << msg->msg_num));
}
EXPORT_SYMBOL(fsl_enable_msg);

void fsl_msg_route_int_to_irqout(struct fsl_msg_unit *msg)
{
	mpic_unmask_irq(msg->irq);
	mpic_irq_set_ep(msg->irq, 1);
}
EXPORT_SYMBOL(fsl_msg_route_int_to_irqout);

void fsl_send_msg(struct fsl_msg_unit *msg, u32 message)
{
	out_be32(msg->msg_addr, message);
}
EXPORT_SYMBOL(fsl_send_msg);

void fsl_read_msg(struct fsl_msg_unit *msg, u32 *message)
{
	*message = in_be32(msg->msg_addr);
}
EXPORT_SYMBOL(fsl_read_msg);

static int __init fsl_init_msg(void)
{
	struct device_node *np;
	struct resource rsrc;

	for_each_compatible_node(np, NULL, "fsl,mpic-msg") {
		int i;
		struct fsl_msg *fsl_msg;

		fsl_msg = kzalloc(sizeof(*fsl_msg), GFP_KERNEL);
		if (!fsl_msg) {
			pr_err("%s: unable to allocate memory\n",
				np->full_name);
			continue;
		}

		of_address_to_resource(np, 0, &rsrc);
		fsl_msg->regs = ioremap(rsrc.start, rsrc.end - rsrc.start);
		if (!fsl_msg->regs) {
			pr_err("%s: unable to iomap registers\n",
			       np->full_name);
			goto err;
		}

		for (i = 0; i < ARRAY_SIZE(fsl_msg->messages); i++) {
			int ret;
			struct resource irq;

			ret = of_irq_to_resource(np, i, &irq);
			if (ret == NO_IRQ) {
				pr_err("%s: not enough interrupts specified\n",
				       np->full_name);
				goto err;
			}
			fsl_msg->messages[i].msg_group_addr_offset =
				rsrc.start & 0xfffff;
			fsl_msg->messages[i].irq = irq.start;
			fsl_msg->messages[i].fsl_msg = fsl_msg;
			fsl_msg->messages[i].msg_num = i;
			fsl_msg->messages[i].mer = &fsl_msg->regs->mer;
			fsl_msg->messages[i].msr = &fsl_msg->regs->msr;
			fsl_msg->messages[i].requested = false;
			fsl_msg->messages[i].msg_addr =
					&fsl_msg->regs->unit[i].addr;
		}
		list_add(&fsl_msg->list_node, &fsl_msgs);

		np->data = fsl_msg;
		of_node_get(np);

		continue;
err:
		kfree(fsl_msg);
	}
	return 0;
}
arch_initcall(fsl_init_msg);
#endif  
