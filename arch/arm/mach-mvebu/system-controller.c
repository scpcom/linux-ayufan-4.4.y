#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/io.h>

static void __iomem *system_controller_base;

struct mvebu_system_controller {
	u32 rstoutn_mask_offset;
	u32 system_soft_reset_offset;

	u32 rstoutn_mask_reset_out_en;
	u32 system_soft_reset;

#if defined(MY_ABC_HERE)
	u32 resume_boot_addr;
#endif  
};
static struct mvebu_system_controller *mvebu_sc;

const struct mvebu_system_controller armada_370_xp_system_controller = {
	.rstoutn_mask_offset = 0x60,
	.system_soft_reset_offset = 0x64,
	.rstoutn_mask_reset_out_en = 0x1,
	.system_soft_reset = 0x1,
};

#if defined(MY_ABC_HERE)
const struct mvebu_system_controller armada_375_system_controller = {
	.rstoutn_mask_offset = 0x54,
	.system_soft_reset_offset = 0x58,
	.rstoutn_mask_reset_out_en = 0x1,
	.system_soft_reset = 0x1,
	.resume_boot_addr = 0xd4,
};
#endif  

const struct mvebu_system_controller orion_system_controller = {
	.rstoutn_mask_offset = 0x108,
	.system_soft_reset_offset = 0x10c,
	.rstoutn_mask_reset_out_en = 0x4,
	.system_soft_reset = 0x1,
};

static struct of_device_id of_system_controller_table[] = {
	{
		.compatible = "marvell,orion-system-controller",
		.data = (void *) &orion_system_controller,
	}, {
		.compatible = "marvell,armada-370-xp-system-controller",
		.data = (void *) &armada_370_xp_system_controller,
#if defined(MY_ABC_HERE)
	}, {
		.compatible = "marvell,armada-375-system-controller",
		.data = (void *) &armada_375_system_controller,
	}, {
		 
		.compatible = "marvell,armada-380-system-controller",
		.data = (void *) &armada_370_xp_system_controller,
#endif  
	},
	{   },
};

void mvebu_restart(char mode, const char *cmd)
{
	if (!system_controller_base) {
		pr_err("Cannot restart, system-controller not available: check the device tree\n");
	} else {
		 
		writel(mvebu_sc->rstoutn_mask_reset_out_en,
			system_controller_base +
			mvebu_sc->rstoutn_mask_offset);
		 
		writel(mvebu_sc->system_soft_reset,
			system_controller_base +
			mvebu_sc->system_soft_reset_offset);
	}

	while (1)
		;
}

#if defined(MY_ABC_HERE)
#ifdef CONFIG_SMP
void armada_375_set_bootaddr(void *boot_addr)
{
	WARN_ON(system_controller_base == NULL);
	writel(virt_to_phys(boot_addr), system_controller_base +
	       mvebu_sc->resume_boot_addr);
}
#endif
#endif  

static int __init mvebu_system_controller_init(void)
{
	struct device_node *np;

	np = of_find_matching_node(NULL, of_system_controller_table);
	if (np) {
		const struct of_device_id *match =
			of_match_node(of_system_controller_table, np);
		BUG_ON(!match);
		system_controller_base = of_iomap(np, 0);
		mvebu_sc = (struct mvebu_system_controller *)match->data;
	}

	return 0;
}

#if defined(MY_ABC_HERE)
early_initcall(mvebu_system_controller_init);
#else  
arch_initcall(mvebu_system_controller_init);
#endif  
