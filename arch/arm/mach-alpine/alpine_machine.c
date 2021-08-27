#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * Device Tree support for Alpine platforms.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#ifdef CONFIG_SYNO_ALPINE
#include <linux/syno.h>
#endif

#include <linux/amba/bus.h>
#include <mach/clkdev.h>
#include <linux/clkdev.h>
#include <linux/export.h>
#include <linux/dma-mapping.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/timer-sp.h>
#include <asm/hardware/arm_timer.h>

#include <mach/al_hal_iomap.h>
#include <mach/al_fabric.h>
#include <mach/alpine_machine.h>
#include <mach/motherboard.h>
#include <mach/system.h>
#include "sched_clock.h"
#include <al_hal/al_hal_serdes.h>
#include <asm/localtimer.h>

#include "core.h"

#define WDTLOAD			0x000
	#define LOAD_MIN	0x00000001
	#define LOAD_MAX	0xFFFFFFFF
#define WDTVALUE		0x004
#define WDTCONTROL		0x008
	/* control register masks */
	#define	INT_ENABLE	(1 << 0)
	#define	RESET_ENABLE	(1 << 1)
#define WDTLOCK			0xC00
	#define	UNLOCK		0x1ACCE551
	#define	LOCK		0x00000001

#define SERDES_NUM_GROUPS	4
#define SERDES_GROUP_SIZE	0x400

#ifdef CONFIG_SYNO_ALPINE
#include <linux/serial_reg.h>
#ifdef SYNO_ALPINE_SUPPORT_WOL
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#endif
#define UART1_REG(x)					(AL_UART_BASE(1) + ((UART_##x) << 2))
#define SET8N1							0x3
#define SOFTWARE_SHUTDOWN				0x31
#define SOFTWARE_REBOOT					0x43

extern void synology_gpio_init(void);

#ifdef SYNO_ALPINE_SUPPORT_WOL
extern void syno_alpine_wol_set();
#endif

static void synology_power_off(void)
{
#ifdef SYNO_ALPINE_SUPPORT_WOL
	syno_alpine_wol_set();
#endif
	printk(KERN_EMERG "Synology shutdown\n");
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_SHUTDOWN, UART1_REG(TX));
}

static void synology_restart(char mode, const char *cmd)
{
	printk(KERN_EMERG "Synology restart\n");
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_REBOOT, UART1_REG(TX));
}
#endif

static void __iomem *wd0_base;
static void __iomem *serdes_base;

static void __init al_timer_init(void);

static struct sys_timer al_timer = {
	.init	= al_timer_init,
};

static const struct of_device_id irq_match[] = {
	{ .compatible = "arm,cortex-a15-gic", .data = gic_of_init, },
	{}
};

static struct clk sp804_clk = {
	.rate = 375000000,
};

static struct clk sb_clk = {
	.rate = 375000000,
};

static struct clk_lookup lookup[] = {
	{	/* AMBA bus clock */
		.con_id		= "apb_pclk",
		.clk		= &sb_clk,
	},
	{
		.dev_id		= "sp804",
		.con_id 	= "al-timer1",
		.clk		= &sp804_clk,
	},
	{
		.dev_id		= "sp804",
		.con_id 	= "al-timer0",
		.clk		= &sp804_clk,
	},
	{
		.dev_id		= "fd880000.i2c-pld",
		.con_id 	= NULL,
		.clk		= &sb_clk,
	},
	{
		.dev_id		= "fd88c000.wdt0",
		.con_id 	= NULL,
		.clk		= &sb_clk,
	},
#ifdef CONFIG_SYNO_ALPINE
	{
		.dev_id		= "fd882000.spi",
		.con_id 	= NULL,
		.clk		= &sb_clk,
	},
#endif
};

static void __init clk_get_freq_dt(unsigned long *refclk, unsigned long *sbclk,
		unsigned long *nbclk, unsigned long *cpuclk)
{
	struct device_node *np;

	/* retrieve the freqency of fixed clocks from device tree */
	for_each_compatible_node(np, NULL, "fixed-clock") {
		u32 rate;
		if (of_property_read_u32(np, "clock-frequency", &rate))
			continue;

		if (!strcmp(np->name, "refclk")) {
			*refclk = rate;
		}
		else if (!strcmp(np->name, "sbclk")) {
			*sbclk = rate;
		}
		else if (!strcmp(np->name, "nbclk")) {
			*nbclk = rate;
		}
		else if (!strcmp(np->name, "cpuclk")) {
			*cpuclk = rate;
		}
	}
}

static inline void al_restart(char mode, const char *cmd)
{
	if (!wd0_base) {
		pr_err("%s: Not supported!\n", __func__);
	} else {
		writel(UNLOCK, wd0_base + WDTLOCK);
		writel(LOAD_MIN, wd0_base + WDTLOAD);
		writel(INT_ENABLE | RESET_ENABLE, wd0_base + WDTCONTROL);
	}

	while (1)
		;
}

static void __init al_timer_init(void)
{
	int irq;
	struct device_node *np;
	void __iomem *timer_base;
	unsigned long refclk, sbclk, nbclk, cpuclk;
	refclk = sbclk = nbclk = cpuclk = 0;

	np = of_find_compatible_node(NULL, NULL, "arm,sp804");
	timer_base = of_iomap(np, 0);
	WARN_ON(!timer_base);
	irq = irq_of_parse_and_map(np, 0);

	clk_get_freq_dt(&refclk, &sbclk, &nbclk, &cpuclk);

	al_sched_clock_init(timer_base + 0x20, sbclk);

	sp804_clocksource_init(timer_base + 0x20, "al-timer1");
	sp804_clockevents_init(timer_base, irq, "al-timer0");

	/* Find the first watchdog and make sure it is not disabled */
	np = of_find_compatible_node(
			NULL, NULL, "arm,sp805");

	if (np && of_device_is_available(np)) {
		wd0_base = of_iomap(np, 0);
		BUG_ON(!wd0_base);
	} else {
		wd0_base = NULL;
	}
}

/* Early initializations */
void __init al_init_early(void)
{
	clkdev_add_table(lookup, ARRAY_SIZE(lookup));
}

static void al_power_off(void)
{
	printk(KERN_EMERG "Unable to shutdown\n");
}

static void __init al_map_io(void)
{
	/* Needed for early printk to work */
	struct map_desc uart_map_desc[1];

	uart_map_desc[0].virtual = (unsigned long)AL_UART_BASE(0);
	uart_map_desc[0].pfn = __phys_to_pfn(AL_UART_BASE(0));
	uart_map_desc[0].length = SZ_64K;
	uart_map_desc[0].type = MT_DEVICE;

	iotable_init(uart_map_desc, ARRAY_SIZE(uart_map_desc));

	/* The 6MB size is plucked from the air */
	init_consistent_dma_size(8 * SZ_1M);
}

static void __init al_init_irq(void)
{
	of_irq_init(irq_match);

	if (al_msix_init() != 0)
		pr_err("%s: al_msix_init() failed!\n", __func__);
}

static void __init al_serdes_resource_init(void)
{
	struct device_node *np;

	/* Find the serdes node and make sure it is not disabled */
	np = of_find_compatible_node(NULL, NULL, "annapurna-labs,al-serdes");

	if (np && of_device_is_available(np)) {
		serdes_base = of_iomap(np, 0);
		BUG_ON(!serdes_base);
	} else {
		pr_err("%s: init serdes regs base failed!\n", __func__);
		serdes_base = NULL;
	}
}

static struct alpine_serdes_eth_group_mode {
#ifdef CONFIG_SYNO_ALPINE_A0
	struct mutex			lock;
#else
	spinlock_t			lock;
#endif
	enum alpine_serdes_eth_mode	mode;
	bool				mode_set;
} alpine_serdes_eth_group_mode[SERDES_NUM_GROUPS] = {
	{
#ifdef CONFIG_SYNO_ALPINE_A0
		.lock = __MUTEX_INITIALIZER(alpine_serdes_eth_group_mode[0].lock),
#else
		.lock = __SPIN_LOCK_UNLOCKED(alpine_serdes_eth_mode_lock_0),
#endif
		.mode_set = false,
	},
	{
#ifdef CONFIG_SYNO_ALPINE_A0
		.lock = __MUTEX_INITIALIZER(alpine_serdes_eth_group_mode[1].lock),
#else
		.lock = __SPIN_LOCK_UNLOCKED(alpine_serdes_eth_mode_lock_1),
#endif
		.mode_set = false,
	},
	{
#ifdef CONFIG_SYNO_ALPINE_A0
		.lock = __MUTEX_INITIALIZER(alpine_serdes_eth_group_mode[2].lock),
#else
		.lock = __SPIN_LOCK_UNLOCKED(alpine_serdes_eth_mode_lock_2),
#endif
		.mode_set = false,
	},
	{
#ifdef CONFIG_SYNO_ALPINE_A0
		.lock = __MUTEX_INITIALIZER(alpine_serdes_eth_group_mode[3].lock),
#else
		.lock = __SPIN_LOCK_UNLOCKED(alpine_serdes_eth_mode_lock_3),
#endif
		.mode_set = false,
	}};

int alpine_serdes_eth_mode_set(
	u32				group,
	enum alpine_serdes_eth_mode	mode)
{
	struct alpine_serdes_eth_group_mode *group_mode =
		&alpine_serdes_eth_group_mode[group];

	if (!serdes_base)
		return -EINVAL;

	if (group >= SERDES_NUM_GROUPS)
		return -EINVAL;

#ifdef CONFIG_SYNO_ALPINE_A0
	mutex_lock(&group_mode->lock);
#else
	spin_lock(&group_mode->lock);
#endif

	if (!group_mode->mode_set || (group_mode->mode != mode)) {
		struct al_serdes_obj obj;

		al_serdes_handle_init(serdes_base, &obj);

		if (mode == ALPINE_SERDES_ETH_MODE_SGMII)
			al_serdes_mode_set_sgmii(&obj, group);
		else
			al_serdes_mode_set_kr(&obj, group);

		group_mode->mode = mode;
		group_mode->mode_set = true;
	}

#ifdef CONFIG_SYNO_ALPINE_A0
	mutex_unlock(&group_mode->lock);
#else
	spin_unlock(&group_mode->lock);
#endif

	return 0;
}
EXPORT_SYMBOL(alpine_serdes_eth_mode_set);

#ifdef CONFIG_SYNO_ALPINE_A0
void alpine_serdes_eth_group_lock(u32 group)
{
	struct alpine_serdes_eth_group_mode *group_mode =
		&alpine_serdes_eth_group_mode[group];

	mutex_lock(&group_mode->lock);
}
EXPORT_SYMBOL(alpine_serdes_eth_group_lock);

void alpine_serdes_eth_group_unlock(u32 group)
{
	struct alpine_serdes_eth_group_mode *group_mode =
		&alpine_serdes_eth_group_mode[group];

	mutex_unlock(&group_mode->lock);
}
EXPORT_SYMBOL(alpine_serdes_eth_group_unlock);
#endif

void __iomem *alpine_serdes_resource_get(u32 group)
{
	void __iomem *base = NULL;

	if (group >= SERDES_NUM_GROUPS)
		return NULL;

	if (serdes_base)
		base = serdes_base + group * SERDES_GROUP_SIZE;

	return base;
}
EXPORT_SYMBOL(alpine_serdes_resource_get);

static void __init al_init(void)
{
#ifndef CONFIG_SYNO_ALPINE
	pm_power_off = al_power_off;
#endif

	/*
	 * Power Management Services Initialization
	 * When running in SMP this should be done earlier
	 */
#ifndef CONFIG_SMP
	alpine_cpu_pm_init();
#endif

	/* fabric uses a notifier for device registration,
	 * Hence it must be initialized before registering
	 * any devices
	 **/
	al_fabric_init();

	al_serdes_resource_init();

	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);

#ifdef CONFIG_SYNO_ALPINE
	pm_power_off = synology_power_off;
	arm_pm_restart = synology_restart;
	synology_gpio_init();
#else
	arm_pm_restart = al_restart;
#endif
}

static const char *al_match[] __initdata = {
	"annapurna-labs,alpine",
	NULL,
};

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
unsigned int al_spin_lock_wfe_enable __read_mostly = 0;
EXPORT_SYMBOL(al_spin_lock_wfe_enable);

static int __init spin_lock_wfe_enable(char *str)
{
	get_option(&str, &al_spin_lock_wfe_enable);
	if (al_spin_lock_wfe_enable)
		al_spin_lock_wfe_enable = 1;
	return 0;
}

early_param("spin_lock_wfe_enable", spin_lock_wfe_enable);
#endif

DT_MACHINE_START(AL_DT, "AnnapurnaLabs Alpine (Device Tree)")
	.map_io		= al_map_io,
	.init_irq	= al_init_irq,
	.timer		= &al_timer,
	.init_machine	= al_init,
	.dt_compat	= al_match,
	.init_early = al_init_early
MACHINE_END
