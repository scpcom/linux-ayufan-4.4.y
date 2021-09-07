 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/of.h>
#include <asm/prom.h>
#include <asm/fsl_lbc.h>

static spinlock_t fsl_lbc_lock = __SPIN_LOCK_UNLOCKED(fsl_lbc_lock);
#ifdef CONFIG_SYNO_QORIQ
struct fsl_lbc_ctrl *fsl_lbc_ctrl_dev;
EXPORT_SYMBOL(fsl_lbc_ctrl_dev);

unsigned int convert_lbc_address(phys_addr_t addr_base)
{
	void *dev;
	int compatible;

	dev = of_find_node_by_name(NULL, "localbus");
	if (!dev) {
		printk(KERN_INFO "fsl-lbc: can't find localbus node\n");
		of_node_put(dev);
		return 0;
	}

	compatible = of_device_is_compatible(dev, "fsl,elbc");
	of_node_put(dev);
	if (compatible)
		return addr_base & 0xffff8000;
	else
		return (addr_base & 0x0ffff8000ull) \
			| ((addr_base & 0x300000000ull) >> 19);
}
EXPORT_SYMBOL(convert_lbc_address);
#else
static struct fsl_lbc_regs __iomem *fsl_lbc_regs;

static char __initdata *compat_lbc[] = {
	"fsl,pq2-localbus",
	"fsl,pq2pro-localbus",
	"fsl,pq3-localbus",
	"fsl,elbc",
};

static int __init fsl_lbc_init(void)
{
	struct device_node *lbus;
	int i;

	for (i = 0; i < ARRAY_SIZE(compat_lbc); i++) {
		lbus = of_find_compatible_node(NULL, NULL, compat_lbc[i]);
		if (lbus)
			goto found;
	}
	return -ENODEV;

found:
	fsl_lbc_regs = of_iomap(lbus, 0);
	of_node_put(lbus);
	if (!fsl_lbc_regs)
		return -ENOMEM;
	return 0;
}
arch_initcall(fsl_lbc_init);
#endif

int fsl_lbc_find(phys_addr_t addr_base)
{
	int i;

#ifdef CONFIG_SYNO_QORIQ
	if (!fsl_lbc_ctrl_dev || !fsl_lbc_ctrl_dev->regs)
#else
	if (!fsl_lbc_regs)
#endif
		return -ENODEV;

#ifdef CONFIG_SYNO_QORIQ
	for (i = 0; i < ARRAY_SIZE(fsl_lbc_ctrl_dev->regs->bank); i++) {
		__be32 br = in_be32(&fsl_lbc_ctrl_dev->regs->bank[i].br);
		__be32 or = in_be32(&fsl_lbc_ctrl_dev->regs->bank[i].or);

		if (br & BR_V && (br & or & BR_BA) \
				== convert_lbc_address(addr_base))
#else
	for (i = 0; i < ARRAY_SIZE(fsl_lbc_regs->bank); i++) {
		__be32 br = in_be32(&fsl_lbc_regs->bank[i].br);
		__be32 or = in_be32(&fsl_lbc_regs->bank[i].or);

		if (br & BR_V && (br & or & BR_BA) == addr_base)
#endif
			return i;
	}

	return -ENOENT;
}
EXPORT_SYMBOL(fsl_lbc_find);

int fsl_upm_find(phys_addr_t addr_base, struct fsl_upm *upm)
{
	int bank;
	__be32 br;

	bank = fsl_lbc_find(addr_base);
	if (bank < 0)
		return bank;

#ifdef CONFIG_SYNO_QORIQ
	if (!fsl_lbc_ctrl_dev || !fsl_lbc_ctrl_dev->regs)
		return -ENODEV;

	br = in_be32(&fsl_lbc_ctrl_dev->regs->bank[bank].br);
#else
	br = in_be32(&fsl_lbc_regs->bank[bank].br);
#endif

	switch (br & BR_MSEL) {
	case BR_MS_UPMA:
#ifdef CONFIG_SYNO_QORIQ
		upm->mxmr = &fsl_lbc_ctrl_dev->regs->mamr;
#else
		upm->mxmr = &fsl_lbc_regs->mamr;
#endif
		break;
	case BR_MS_UPMB:
#ifdef CONFIG_SYNO_QORIQ
		upm->mxmr = &fsl_lbc_ctrl_dev->regs->mbmr;
#else
		upm->mxmr = &fsl_lbc_regs->mbmr;
#endif
		break;
	case BR_MS_UPMC:
#ifdef CONFIG_SYNO_QORIQ
		upm->mxmr = &fsl_lbc_ctrl_dev->regs->mcmr;
#else
		upm->mxmr = &fsl_lbc_regs->mcmr;
#endif
		break;
	default:
		return -EINVAL;
	}

	switch (br & BR_PS) {
	case BR_PS_8:
		upm->width = 8;
		break;
	case BR_PS_16:
		upm->width = 16;
		break;
	case BR_PS_32:
		upm->width = 32;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(fsl_upm_find);

int fsl_upm_run_pattern(struct fsl_upm *upm, void __iomem *io_base, u32 mar)
{
	int ret = 0;
	unsigned long flags;

#ifdef CONFIG_SYNO_QORIQ
	if (!fsl_lbc_ctrl_dev || !fsl_lbc_ctrl_dev->regs)
		return -ENODEV;
#endif

	spin_lock_irqsave(&fsl_lbc_lock, flags);

#ifdef CONFIG_SYNO_QORIQ
	out_be32(&fsl_lbc_ctrl_dev->regs->mar, mar);
#else
	out_be32(&fsl_lbc_regs->mar, mar);
#endif

	switch (upm->width) {
	case 8:
		out_8(io_base, 0x0);
		break;
	case 16:
		out_be16(io_base, 0x0);
		break;
	case 32:
		out_be32(io_base, 0x0);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock_irqrestore(&fsl_lbc_lock, flags);

	return ret;
}
EXPORT_SYMBOL(fsl_upm_run_pattern);

#ifdef CONFIG_SYNO_QORIQ
static int __devinit fsl_lbc_ctrl_init(struct fsl_lbc_ctrl *ctrl)
{
	struct fsl_lbc_regs __iomem *lbc = ctrl->regs;

	clrsetbits_be32(&lbc->lbcr, LBCR_BMT, 15);

	setbits32(&lbc->ltesr, LTESR_CLEAR);
	out_be32(&lbc->lteatr, 0);
	out_be32(&lbc->ltedr, LTEDR_ENABLE);

	out_be32(&lbc->lteir, LTEIR_ENABLE);

	return 0;
}

static int __devexit fsl_lbc_ctrl_remove(struct of_device *ofdev)
{
	struct fsl_lbc_ctrl *ctrl = dev_get_drvdata(&ofdev->dev);

	if (ctrl->irq)
		free_irq(ctrl->irq, ctrl);

	if (ctrl->regs)
		iounmap(ctrl->regs);

	dev_set_drvdata(&ofdev->dev, NULL);
	kfree(ctrl);

	return 0;
}

static irqreturn_t fsl_lbc_ctrl_irq(int irqno, void *data)
{
	struct fsl_lbc_ctrl *ctrl = data;
	struct fsl_lbc_regs __iomem *lbc = ctrl->regs;
	u32 status;

	status = in_be32(&lbc->ltesr);

	if (status) {
		out_be32(&lbc->ltesr, LTESR_CLEAR);
		out_be32(&lbc->lteatr, 0);
		ctrl->irq_status = status;

		if (status & LTESR_BM)
			dev_err(ctrl->dev, "Local bus monitor time-out: "
				"LTESR 0x%08X\n", status);
		if (status & LTESR_WP)
			dev_err(ctrl->dev, "Write protect error: "
				"LTESR 0x%08X\n", status);
		if (status & LTESR_ATMW)
			dev_err(ctrl->dev, "Atomic write error: "
				"LTESR 0x%08X\n", status);
		if (status & LTESR_ATMR)
			dev_err(ctrl->dev, "Atomic read error: "
				"LTESR 0x%08X\n", status);
		if (status & LTESR_CS)
			dev_err(ctrl->dev, "Chip select error: "
				"LTESR 0x%08X\n", status);
		if (status & LTESR_UPM)
			;
		if (status & LTESR_FCT) {
			dev_err(ctrl->dev, "FCM command time-out: "
				"LTESR 0x%08X\n", status);
			smp_wmb();
			wake_up(&ctrl->irq_wait);
		}
		if (status & LTESR_PAR) {
			dev_err(ctrl->dev, "Parity or Uncorrectable ECC error: "
				"LTESR 0x%08X\n", status);
			smp_wmb();
			wake_up(&ctrl->irq_wait);
		}
		if (status & LTESR_CC) {
			smp_wmb();
			wake_up(&ctrl->irq_wait);
		}
		if (status & ~LTESR_MASK)
			dev_err(ctrl->dev, "Unknown error: "
				"LTESR 0x%08X\n", status);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int __devinit fsl_lbc_ctrl_probe(struct of_device *ofdev,
					 const struct of_device_id *match)
{
	int ret;

	fsl_lbc_ctrl_dev = kzalloc(sizeof(*fsl_lbc_ctrl_dev), GFP_KERNEL);
	if (!fsl_lbc_ctrl_dev)
		return -ENOMEM;

	dev_set_drvdata(&ofdev->dev, fsl_lbc_ctrl_dev);

	spin_lock_init(&fsl_lbc_ctrl_dev->lock);
	init_waitqueue_head(&fsl_lbc_ctrl_dev->irq_wait);

	fsl_lbc_ctrl_dev->regs = of_iomap(ofdev->node, 0);
	if (!fsl_lbc_ctrl_dev->regs) {
		dev_err(&ofdev->dev, "failed to get memory region\n");
		ret = -ENODEV;
		goto err;
	}

	fsl_lbc_ctrl_dev->irq = of_irq_to_resource(ofdev->node, 0, NULL);
	if (fsl_lbc_ctrl_dev->irq == NO_IRQ) {
		dev_err(&ofdev->dev, "failed to get irq resource\n");
		ret = -ENODEV;
		goto err;
	}

	fsl_lbc_ctrl_dev->dev = &ofdev->dev;

	ret = fsl_lbc_ctrl_init(fsl_lbc_ctrl_dev);
	if (ret < 0)
		goto err;

	ret = request_irq(fsl_lbc_ctrl_dev->irq, fsl_lbc_ctrl_irq, 0,
				"fsl-lbc", fsl_lbc_ctrl_dev);
	if (ret != 0) {
		dev_err(&ofdev->dev, "failed to install irq (%d)\n",
			fsl_lbc_ctrl_dev->irq);
		ret = fsl_lbc_ctrl_dev->irq;
		goto err;
	}

	return 0;

err:
	return ret;
}

#ifdef CONFIG_SUSPEND
static struct fsl_lbc_regs lbc_saveed_regs;
#define COUNT_OF_BANK_P1022 8
#define COUNT_OF_BANKS COUNT_OF_BANK_P1022

static int fsl_lbc_suspend(struct of_device *ofdev, pm_message_t state)
{
	struct fsl_lbc_ctrl *ctrl = dev_get_drvdata(&ofdev->dev);
	struct fsl_lbc_regs __iomem *lbc = ctrl->regs;
	int i;

	for (i = 0; i < COUNT_OF_BANKS; i++) {
		lbc_saveed_regs.bank[i].br =
			in_be32(&lbc->bank[i].br);
		lbc_saveed_regs.bank[i].or =
			in_be32(&lbc->bank[i].or);
	}
	lbc_saveed_regs.mar = in_be32(&lbc->mar);
	lbc_saveed_regs.mamr = in_be32(&lbc->mamr);
	lbc_saveed_regs.mbmr = in_be32(&lbc->mbmr);
	lbc_saveed_regs.mcmr = in_be32(&lbc->mcmr);
	lbc_saveed_regs.mrtpr = in_be32(&lbc->mrtpr);
	lbc_saveed_regs.mdr = in_be32(&lbc->mdr);
	lbc_saveed_regs.lsor = in_be32(&lbc->lsor);
	lbc_saveed_regs.lsdmr = in_be32(&lbc->lsdmr);
	lbc_saveed_regs.lurt = in_be32(&lbc->lurt);
	lbc_saveed_regs.lsrt = in_be32(&lbc->lsrt);
	lbc_saveed_regs.ltedr = in_be32(&lbc->ltedr);
	lbc_saveed_regs.lteir = in_be32(&lbc->lteir);
	lbc_saveed_regs.lteatr = in_be32(&lbc->lteatr);
	lbc_saveed_regs.ltear = in_be32(&lbc->ltear);
	lbc_saveed_regs.lbcr = in_be32(&lbc->lbcr);
	lbc_saveed_regs.lcrr = in_be32(&lbc->lcrr);
	lbc_saveed_regs.fmr = in_be32(&lbc->fmr);
	lbc_saveed_regs.fir = in_be32(&lbc->fir);
	lbc_saveed_regs.fcr = in_be32(&lbc->fcr);
	lbc_saveed_regs.fbar = in_be32(&lbc->fbar);
	lbc_saveed_regs.fpar = in_be32(&lbc->fpar);
	lbc_saveed_regs.fbcr = in_be32(&lbc->fbcr);

	return 0;
}

static int fsl_lbc_resume(struct of_device *ofdev)
{
	struct fsl_lbc_ctrl *ctrl = dev_get_drvdata(&ofdev->dev);
	struct fsl_lbc_regs __iomem *lbc = ctrl->regs;
	int i;

	for (i = 0; i < COUNT_OF_BANKS; i++) {
		out_be32(&lbc->bank[i].br,
				lbc_saveed_regs.bank[i].br);
		out_be32(&lbc->bank[i].or,
				lbc_saveed_regs.bank[i].or);
	}

	out_be32(&lbc->mar, lbc_saveed_regs.mar);
	out_be32(&lbc->mamr, lbc_saveed_regs.mamr);
	out_be32(&lbc->mbmr, lbc_saveed_regs.mbmr);
	out_be32(&lbc->mcmr, lbc_saveed_regs.mcmr);
	out_be32(&lbc->mrtpr, lbc_saveed_regs.mrtpr);
	out_be32(&lbc->mdr, lbc_saveed_regs.mdr);
	out_be32(&lbc->lsor, lbc_saveed_regs.lsor);
	out_be32(&lbc->lsdmr, lbc_saveed_regs.lsdmr);
	out_be32(&lbc->lurt, lbc_saveed_regs.lurt);
	out_be32(&lbc->lsrt, lbc_saveed_regs.lsrt);
	out_be32(&lbc->ltedr, lbc_saveed_regs.ltedr);
	out_be32(&lbc->lteir, lbc_saveed_regs.lteir);
	out_be32(&lbc->lteatr, lbc_saveed_regs.lteatr);
	out_be32(&lbc->ltear, lbc_saveed_regs.ltear);
	out_be32(&lbc->lbcr, lbc_saveed_regs.lbcr);
	out_be32(&lbc->lcrr, lbc_saveed_regs.lcrr);
	out_be32(&lbc->fmr, lbc_saveed_regs.fmr);
	out_be32(&lbc->fir, lbc_saveed_regs.fir);
	out_be32(&lbc->fcr, lbc_saveed_regs.fcr);
	out_be32(&lbc->fbar, lbc_saveed_regs.fbar);
	out_be32(&lbc->fpar, lbc_saveed_regs.fpar);
	out_be32(&lbc->fbcr, lbc_saveed_regs.fbcr);

	return 0;
}
#endif  

static const struct of_device_id fsl_lbc_match[] = {
	{
		.compatible = "fsl,elbc",
	},
	{
		.compatible = "fsl,pq3-localbus",
	},
	{
		.compatible = "fsl,pq2-localbus",
	},
	{
		.compatible = "fsl,pq2pro-localbus",
	},
	{},
};

static struct of_platform_driver fsl_lbc_ctrl_driver = {
	.driver = {
		.name	= "fsl-lbc",
	},
	.match_table = fsl_lbc_match,
	.probe       = fsl_lbc_ctrl_probe,
	.remove      = __devexit_p(fsl_lbc_ctrl_remove),
#ifdef CONFIG_SUSPEND
	.suspend     = fsl_lbc_suspend,
	.resume      = fsl_lbc_resume,
#endif
};

static int __init fsl_lbc_init(void)
{
	return of_register_platform_driver(&fsl_lbc_ctrl_driver);
}

static void __exit fsl_lbc_exit(void)
{
	of_unregister_platform_driver(&fsl_lbc_ctrl_driver);
}

module_init(fsl_lbc_init);
module_exit(fsl_lbc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("Freescale Enhanced Local Bus Controller driver");
#endif
