#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/cnt32_to_63.h>
#include <linux/io.h>

#include <linux/clkdev.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/hardware/arm_timer.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/sched_clock.h>
#include <mach/hardware.h>
#include <mach/early-debug.h>
#include <mach/irqs.h>
#include <linux/irqchip/arm-gic.h>
#include "mach/clock.h"

#include <linux/bootmem.h>
#include <linux/delay.h>
#include <asm/smp_twd.h>
#include <linux/memblock.h>

#include "platsmp.h"
#include <asm/cacheflush.h>
#include <asm/setup.h>

#define A17_AXI_SCALE_REG   IO_ADDRESS(REG_BASE_CRG + 0x30)
#define REG_PERI_CRG57     IO_ADDRESS(REG_BASE_CRG + 0xe4)

#define get_bus_clk()({\
		unsigned long tmp_reg, busclk;\
		tmp_reg = readl((volatile void *)A17_AXI_SCALE_REG);\
		tmp_reg = tmp_reg & 0x3;\
		if (tmp_reg  == 0x1)\
			busclk = 300000000;\
		else if (tmp_reg == 0x2)\
			busclk = 250000000;\
		else if (tmp_reg == 0x3)\
			busclk = 200000000;\
		else\
			busclk = 200000000;\
		busclk;\
		})

void __iomem *hi3536_gic_cpu_base_addr = IOMEM(CFG_GIC_CPU_BASE);

unsigned long long hi_sched_clock(void)
{
	return sched_clock();
}
EXPORT_SYMBOL(hi_sched_clock);

void __init hi3536_gic_init_irq(void)
{
	edb_trace();
#ifdef CONFIG_HI3536_SYSCNT
	gic_init(0, IRQ_LOCALTIMER, IOMEM(CFG_GIC_DIST_BASE),
			IOMEM(CFG_GIC_CPU_BASE));

#else
#ifndef CONFIG_LOCAL_TIMERS
	gic_init(0, HISI_GIC_IRQ_START, IOMEM(CFG_GIC_DIST_BASE),
			IOMEM(CFG_GIC_CPU_BASE));
#else
	/*
	 * git initialed include Local timer.
	 * IRQ_LOCALTIMER is settled IRQ number for local timer interrupt.
	 * It is set to 29 by ARM.
	 */
	gic_init(0, HISI_GIC_IRQ_START, IOMEM(CFG_GIC_DIST_BASE),
			IOMEM(CFG_GIC_CPU_BASE));
#endif
#endif
}

/*******************************************************************/
/*
 * copy startup code to sram, and flash cache.
 * @start_addr: slave start phy address
 * @jump_addr: slave jump phy address
 */
void set_scu_boot_addr(unsigned int start_addr, unsigned int jump_addr)
{
	unsigned int *virtaddr;
	unsigned int *p_virtaddr;

	p_virtaddr = virtaddr = ioremap(start_addr, PAGE_SIZE);

	if (NULL == p_virtaddr) {
		pr_err("slave core start  failed!\n");
		return;
	}

	*p_virtaddr++ = 0xe51ff004; /* ldr  pc, [pc, #-4] */
	*p_virtaddr++ = jump_addr;  /* pc jump phy address */

	smp_wmb();
	__cpuc_flush_dcache_area((void *)virtaddr,
			(size_t)((char *)p_virtaddr - (char *)virtaddr));
	outer_clean_range(__pa(virtaddr), __pa(p_virtaddr));

	iounmap(virtaddr);
}

#ifdef CONFIG_A17_BOOT_A7
unsigned int slave_boot_addr;

static int hi3536_parse_tag(const struct tag *tag)
{
	char *s = NULL;
	int ret = 0;

	if (tag->hdr.size != 8) {
		pr_err("hi3536 tag size error!\n");
		return 0;
	}

	s = (char *)&tag->u;
	ret = kstrtoul(s, 16, (unsigned long *)&slave_boot_addr);
	if (ret)
		return ret;

	return slave_boot_addr;

}
__tagtable(CONFIG_HI3536_TAG, hi3536_parse_tag);

void __init hi3536_boot_second_os(void)
{
	unsigned int   regval;

	if (!slave_boot_addr) {
		pr_err("slave boot addr is error!\n");
		return;
	}

	set_scu_boot_addr(0x00000000, (unsigned int)(slave_boot_addr));

	/* clear the A7 cpu reset */
	regval = readl(__io_address(REG_BASE_CRG + REG_PERI_CRG98));
	regval &= ~(0x1 << 5);
	writel(regval, __io_address(REG_BASE_CRG + REG_PERI_CRG98));
}
#endif

static struct map_desc hi3536_io_desc[] __initdata = {
	/* hi3536_IOCH1 */
	{
		.virtual	= HI3536_IOCH1_VIRT,
		.pfn		= __phys_to_pfn(HI3536_IOCH1_PHYS),
		.length		= HI3536_IOCH1_SIZE,
		.type		= MT_DEVICE
	},
	/* hi3536_IOCH2 */
	{
		.virtual        = HI3536_IOCH2_VIRT,
		.pfn            = __phys_to_pfn(HI3536_IOCH2_PHYS),
		.length         = HI3536_IOCH2_SIZE,
		.type           = MT_DEVICE
	},
};

void __init hi3536_map_io(void)
{
	int i;

	iotable_init(hi3536_io_desc, ARRAY_SIZE(hi3536_io_desc));

	edb_trace();
	for (i = 0; i < ARRAY_SIZE(hi3536_io_desc); i++) {
		edb_putstr(" V: ");	edb_puthex(hi3536_io_desc[i].virtual);
		edb_putstr(" P: ");	edb_puthex(hi3536_io_desc[i].pfn);
		edb_putstr(" S: ");	edb_puthex(hi3536_io_desc[i].length);
		edb_putstr(" T: ");	edb_putul(hi3536_io_desc[i].type);
		edb_putstr("\n");
	}

	edb_trace();
}

#define HIL_AMBADEV_NAME(name) hil_ambadevice_##name

#define HIL_AMBA_DEVICE(name, busid, base, platdata)		\
	static struct amba_device HIL_AMBADEV_NAME(name) =		\
{								\
	.dev		= {					\
		.coherent_dma_mask = ~0,			\
		.init_name = busid,				\
		.platform_data = platdata,			\
	},							\
	.res		= {					\
		.start	= REG_BASE_##base,			\
		.end	= REG_BASE_##base + 0x1000 - 1,		\
		.flags	= IORESOURCE_IO,			\
	},							\
	.dma_mask	= ~0,					\
	.irq		= { INTNR_##base, INTNR_##base }	\
}

HIL_AMBA_DEVICE(uart0, "uart:0",  UART0,    NULL);
HIL_AMBA_DEVICE(uart1, "uart:1",  UART1,    NULL);

static struct amba_device *amba_devs[] __initdata = {
	&HIL_AMBADEV_NAME(uart0),
	&HIL_AMBADEV_NAME(uart1),
};

/*****************************************************************************/
/*
 * These are fixed clocks.
 */
#ifdef CONFIG_HI3536_SYSCNT
static struct clk syscnt_clk = {
	.rate	= 24000000,
};
#endif

static struct clk uart_clk = {
	.rate = 2000000,
};

static struct clk sp804_clk = {
	.rate	= 25000000, /* TODO: XXX */
};

static struct clk_lookup lookups[] = {
	{
		/* UART0 */
		.dev_id		= "uart:0",
		.clk		= &uart_clk,
	},
	{
		/* UART1 */
		.dev_id		= "uart:1",
		.clk		= &uart_clk,
	},
	{ /* SP804 timers */
		.dev_id		= "sp804",
		.clk		= &sp804_clk,
	},
#ifdef CONFIG_HI3536_SYSCNT
	{ /* syscnt timers */
		.dev_id		= "syscnt",
		.clk		= &syscnt_clk,
	},
#endif
};

static void __init hi3536_reserve(void)
{
}

void __init hi3536_init(void)
{
	unsigned long i;

	edb_trace();

	for (i = 0; i < ARRAY_SIZE(amba_devs); i++) {
		edb_trace();
		amba_device_register(amba_devs[i], &iomem_resource);
	}

#ifdef CONFIG_A17_BOOT_A7
	/* A17 boot A7 */
	hi3536_boot_second_os();
#endif
}

static void __init hi3536_init_early(void)
{

	unsigned long reg = 0;

	/* hi3536 ASIC uart use apb bus clk */
	reg = readl((volatile void *)REG_PERI_CRG57);
	reg &= ~UART_CKSEL_APB;
	writel(reg, (volatile void *)REG_PERI_CRG57);

	uart_clk.rate = get_bus_clk() / 2;
	sp804_clk.rate = get_bus_clk() / 2;

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	/*
	 * 1. enable L1 prefetch                       [2]
	 * 4. enable allocation in one cache way only. [8]
	 */
	asm volatile (
			"	mrc	p15, 0, r0, c1, c0, 1\n"
			"	orr	r0, r0, #0x104\n"
			"	mcr	p15, 0, r0, c1, c0, 1\n"
			:
			:
			: "r0", "cc");

	edb_trace();
}

void hi3536_restart(char mode, const char *cmd)
{
	writel(~0, __io_address(REG_BASE_SCTL + REG_SC_SYSRES));
}

extern void __init hi3536_timer_init(void);
#ifdef CONFIG_HI3536_SYSCNT
extern void __init arch_timer_init(void);
#endif

MACHINE_START(HI3536, "hi3536")
.atag_offset  = 0x100,
	.map_io       = hi3536_map_io,
	.init_early   = hi3536_init_early,
	.init_irq     = hi3536_gic_init_irq,
#ifdef CONFIG_HI3536_SYSCNT
	.init_time    = arch_timer_init,
#else
	.init_time    = hi3536_timer_init,
#endif
	.init_machine = hi3536_init,
	.smp          = smp_ops(hi3536_smp_ops),
	.reserve      = hi3536_reserve,
	.restart      = hi3536_restart,
	MACHINE_END
