#ifdef CONFIG_SYNO_QORIQ
 
#include <linux/kernel.h>
#include <linux/of_platform.h>
#include <asm/io.h>

#include "fsl_85xx_cache_ctlr.h"

static char *param;
struct mpc85xx_l2ctlr __iomem *l2ctlr;

static long get_cache_sram_size(void)
{
	unsigned long val;

	if (!param || (strict_strtoul(param, 0, &val) < 0))
		return -EINVAL;

	return val;
}

static int __init get_cmdline_param(char *str)
{
	if (!str)
		return 0;

	param = str;
	return 1;
}

__setup("cache-sram-size=", get_cmdline_param);

static int __devinit mpc85xx_l2ctlr_of_probe(struct of_device *dev,
					  const struct of_device_id *match)
{
	long rval;
	unsigned int rem;
	unsigned char ways;
	const unsigned int *prop;
	unsigned int l2cache_size;
	unsigned int sram_size;
	struct device_node *np;
	int rc = 0;
	struct resource rsrc;

	if (!dev->node) {
		dev_err(&dev->dev, "Device's OF-node is NULL\n");
		return -EINVAL;
	}

	prop = of_get_property(dev->node, "cache-size", NULL);
	if (!prop) {
		dev_err(&dev->dev, "Missing L2 cache-size\n");
		return -EINVAL;
	}
	l2cache_size = *prop;

	rval = get_cache_sram_size();
	if (rval <= 0) {
		dev_err(&dev->dev,
			"Entire L2 as cache, Aborting Cache-SRAM stuff\n");
		return -EINVAL;
	}

	rem = l2cache_size % (unsigned int)rval;
	ways = LOCK_WAYS_FULL * (unsigned int)rval / l2cache_size;
	if (rem || (ways & (ways - 1))) {
		dev_err(&dev->dev, "Illegal cache-sram-size in command line\n");
		return -EINVAL;
	}

	sram_size = (unsigned int)rval;

	l2ctlr = of_iomap(dev->node, 0);
	if (!l2ctlr) {
		dev_err(&dev->dev, "Can't map L2 controller\n");
		return -EINVAL;
	}

	for_each_compatible_node(np, NULL, "fsl,l2sram")
			rc = of_address_to_resource(np, 0, &rsrc);

	if (rc) {
		printk(KERN_ERR "Can't get %s property 'reg'\n", np->name);
		return -EFAULT;
	}

	out_be32(&l2ctlr->srbar0,
		rsrc.start & L2SRAM_BAR_MSK_LO18);

#ifdef CONFIG_PHYS_64BIT
	out_be32(&l2ctlr->srbarea0,
		(rsrc.start >> 32) & L2SRAM_BARE_MSK_HI4);
#endif

	rsrc.end = rsrc.start + sram_size - 1;

	clrsetbits_be32(&l2ctlr->ctl, L2CR_L2E, L2CR_L2FI);

	switch (ways) {
	case LOCK_WAYS_EIGHTH:
		setbits32(&l2ctlr->ctl,
			L2CR_L2E | L2CR_L2FI | L2CR_SRAM_EIGHTH);
		break;

	case LOCK_WAYS_TWO_EIGHTH:
		setbits32(&l2ctlr->ctl,
			L2CR_L2E | L2CR_L2FI | L2CR_SRAM_QUART);
		break;

	case LOCK_WAYS_HALF:
		setbits32(&l2ctlr->ctl,
			L2CR_L2E | L2CR_L2FI | L2CR_SRAM_HALF);
		break;

	case LOCK_WAYS_FULL:
	default:
		setbits32(&l2ctlr->ctl,
			L2CR_L2E | L2CR_L2FI | L2CR_SRAM_FULL);
		break;
	}
	eieio();

	rval = instantiate_cache_sram(dev, &rsrc);
	if (rval < 0) {
		dev_err(&dev->dev, "Can't instantiate Cache-SRAM\n");
		iounmap(l2ctlr);
		return -EINVAL;
	}

	return 0;
}

static int __devexit mpc85xx_l2ctlr_of_remove(struct of_device *dev)
{
	BUG_ON(!l2ctlr);

	iounmap(l2ctlr);
	remove_cache_sram(dev);
	dev_info(&dev->dev, "MPC85xx L2 controller unloaded\n");

	return 0;
}

static struct of_device_id mpc85xx_l2ctlr_of_match[] = {
	{
		.compatible = "fsl,p2020-l2-cache-controller",
	},
	{},
};

static struct of_platform_driver mpc85xx_l2ctlr_of_platform_driver = {
	.name		= "fsl-l2ctlr",
	.match_table	= mpc85xx_l2ctlr_of_match,
	.probe		= mpc85xx_l2ctlr_of_probe,
	.remove		= __devexit_p(mpc85xx_l2ctlr_of_remove),
};

static __init int mpc85xx_l2ctlr_of_init(void)
{
	return of_register_platform_driver(&mpc85xx_l2ctlr_of_platform_driver);
}

static void __exit mpc85xx_l2ctlr_of_exit(void)
{
	of_unregister_platform_driver(&mpc85xx_l2ctlr_of_platform_driver);
}

subsys_initcall(mpc85xx_l2ctlr_of_init);
module_exit(mpc85xx_l2ctlr_of_exit);

MODULE_DESCRIPTION("Freescale MPC85xx L2 controller init");
MODULE_LICENSE("GPL v2");
#endif  
