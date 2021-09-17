#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/cpu_pm.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mbus.h>
#include <linux/of_address.h>
#include <linux/suspend.h>
#if defined(MY_ABC_HERE)
#include <linux/regulator/machine.h>
#endif  
#include <asm/cacheflush.h>
#include <asm/outercache.h>
#include <asm/suspend.h>

#include "coherency.h"
#include "pmsu.h"

#define SDRAM_CONFIG_OFFS                  0x0
#define  SDRAM_CONFIG_SR_MODE_BIT          BIT(24)
#define SDRAM_OPERATION_OFFS               0x18
#define  SDRAM_OPERATION_SELF_REFRESH      0x7
#define SDRAM_DLB_EVICTION_OFFS            0x30c
#define  SDRAM_DLB_EVICTION_THRESHOLD_MASK 0xff

extern void armada_38x_cpu_mem_resume(void);
#if defined(MY_ABC_HERE)
 
#else  
extern int mvebu_pcie_resume(void);
#endif  

static void (*mvebu_board_pm_enter)(void __iomem *sdram_reg, u32 srcmd);
static void __iomem *sdram_ctrl;
#if defined(MY_ABC_HERE)
static bool is_suspend_mem_available;
static int (*mvebu_board_pm_init)(void);
#endif  

static int mvebu_pm_powerdown(unsigned long data)
{
	u32 reg, srcmd;

	flush_cache_all();
	outer_flush_all();

	dsb();

	reg = readl(sdram_ctrl + SDRAM_DLB_EVICTION_OFFS);
	reg &= ~SDRAM_DLB_EVICTION_THRESHOLD_MASK;
	writel(reg, sdram_ctrl + SDRAM_DLB_EVICTION_OFFS);

	udelay(7);

	reg = readl(sdram_ctrl + SDRAM_CONFIG_OFFS);
	reg &= ~SDRAM_CONFIG_SR_MODE_BIT;
	writel(reg, sdram_ctrl + SDRAM_CONFIG_OFFS);

	srcmd = readl(sdram_ctrl + SDRAM_OPERATION_OFFS);
	srcmd &= ~0x1F;
	srcmd |= SDRAM_OPERATION_SELF_REFRESH;

	mvebu_board_pm_enter(sdram_ctrl + SDRAM_OPERATION_OFFS, srcmd);

	return 0;
}

#define BOOT_INFO_ADDR      0x3000
#define BOOT_MAGIC_WORD	    0xdeadb002
#define BOOT_MAGIC_LIST_END 0xffffffff

#define MBUS_WINDOW_12_CTRL       0xd00200b0
#define MBUS_INTERNAL_REG_ADDRESS 0xd0020080

#define SDRAM_WIN_BASE_REG(x)	(0x20180 + (0x8*x))
#define SDRAM_WIN_CTRL_REG(x)	(0x20184 + (0x8*x))

#if defined(MY_ABC_HERE)
 
#else  
static phys_addr_t mvebu_internal_reg_base(void)
{
	struct device_node *np;
	__be32 in_addr[2];

	np = of_find_node_by_name(NULL, "internal-regs");
	BUG_ON(!np);

	in_addr[0] = cpu_to_be32(0xf0010000);
	in_addr[1] = 0x0;

	return of_translate_address(np, in_addr);
}
#endif  

static void mvebu_pm_store_bootinfo(void)
{
	u32 *store_addr;
	phys_addr_t resume_pc;

	store_addr = phys_to_virt(BOOT_INFO_ADDR);
	 
	resume_pc = virt_to_phys(armada_38x_cpu_mem_resume);

	writel(BOOT_MAGIC_WORD, store_addr++);
	writel(resume_pc, store_addr++);

	writel(MBUS_WINDOW_12_CTRL, store_addr++);
	writel(0x0, store_addr++);

#if 0
	writel(MBUS_INTERNAL_REG_ADDRESS, store_addr++); */
	writel(mvebu_internal_reg_base(), store_addr++);
#endif

	store_addr += mvebu_mbus_save_cpu_target(store_addr);

	writel(BOOT_MAGIC_LIST_END, store_addr);
}

#if defined(MY_ABC_HERE)
static void mvebu_enter_suspend(void)
#else  
static int mvebu_pm_enter(suspend_state_t state)
#endif  
{
#if defined(MY_ABC_HERE)
	 
#else  
	if (state != PM_SUSPEND_MEM)
		return -EINVAL;
#endif  

	cpu_pm_enter();

	mvebu_pm_store_bootinfo();

	outer_flush_all();
	outer_disable();

	cpu_suspend(0, mvebu_pm_powerdown);

#if defined(MY_ABC_HERE)
	 
	mvebu_v7_pmsu_disable_dfs_cpu(1);
#endif  

	outer_disable();
	outer_resume();

	mvebu_v7_pmsu_idle_exit();

	set_cpu_coherent();

#if defined(MY_ABC_HERE)
	 
#else  
	 
	mvebu_pcie_resume();
#endif  

	cpu_pm_exit();
#if defined(MY_ABC_HERE)
	 
#else  
	return 0;
#endif  
}

#if defined(MY_ABC_HERE)
static int mvebu_pm_enter(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_STANDBY:
		cpu_do_idle();
		break;
	case PM_SUSPEND_MEM:
		mvebu_enter_suspend();
	default:
		return -EINVAL;
	}
	return 0;
}

static int mvebu_pm_valid(suspend_state_t state)
{
	return ((state == PM_SUSPEND_STANDBY) ||
		(is_suspend_mem_available && (state == PM_SUSPEND_MEM)));
}

#if defined(MY_ABC_HERE)
static int mvebu_suspend_prepare(void)
{
	int ret;

	ret = regulator_suspend_prepare(PM_SUSPEND_MEM);
	if (ret)
		pr_err("Failed to prepare regulators for PM_SUSPEND_MEM (%d)\n", ret);

	return ret;
}
#endif  

static void mvebu_suspend_finish(void)
{
	int ret;

	ret = regulator_suspend_finish();
	if (ret)
		pr_warn("Failed to resume regulators from suspend (%d)\n", ret);
}
#endif  

static const struct platform_suspend_ops mvebu_pm_ops = {
	.enter = mvebu_pm_enter,
#if defined(MY_ABC_HERE) || \
	defined(MY_ABC_HERE)
	.valid = mvebu_pm_valid,
	.finish = mvebu_suspend_finish,
#if defined(MY_ABC_HERE)
	.prepare = mvebu_suspend_prepare,
#endif  
#else
	.valid = suspend_valid_only_mem,
#endif
};

#if defined(MY_ABC_HERE)
void __init mvebu_pm_register_init(int (*board_pm_init)(void))
{
	mvebu_board_pm_init = board_pm_init;
}

static int __init mvebu_pm_init(void)
{

	if (!of_machine_is_compatible("marvell,armadaxp") &&
			!of_machine_is_compatible("marvell,armada370") &&
			!of_machine_is_compatible("marvell,armada38x") &&
			!of_machine_is_compatible("marvell,armada390"))
		return -ENODEV;

	if (mvebu_board_pm_init)
		if (mvebu_board_pm_init())
			pr_warn("Registering suspend init for this board failed\n");
	suspend_set_ops(&mvebu_pm_ops);

	return 0;
}
#endif  

#if defined(MY_ABC_HERE)
int __init mvebu_pm_suspend_init(void (*board_pm_enter)(void __iomem *sdram_reg,
								u32 srcmd))
#else  
int mvebu_pm_init(void (*board_pm_enter)(void __iomem *sdram_reg, u32 srcmd))
#endif  
{
	struct device_node *np;
	struct resource res;

	if (!of_machine_is_compatible("marvell,armadaxp") &&
		!of_machine_is_compatible("marvell,armada38x"))
		return -ENODEV;

	np = of_find_compatible_node(NULL, NULL,
				     "marvell,armada-xp-sdram-controller");
	if (!np)
		return -ENODEV;

	if (of_address_to_resource(np, 0, &res)) {
		of_node_put(np);
		return -ENODEV;
	}

	if (!request_mem_region(res.start, resource_size(&res),
				np->full_name)) {
		of_node_put(np);
		return -EBUSY;
	}

	sdram_ctrl = ioremap(res.start, resource_size(&res));
	if (!sdram_ctrl) {
		release_mem_region(res.start, resource_size(&res));
		of_node_put(np);
		return -ENOMEM;
	}

	of_node_put(np);

	mvebu_board_pm_enter = board_pm_enter;

#if defined(MY_ABC_HERE)
	is_suspend_mem_available = true;
#else  
	suspend_set_ops(&mvebu_pm_ops);
#endif  

	return 0;
}

#if defined(MY_ABC_HERE)
late_initcall(mvebu_pm_init);
#endif  
