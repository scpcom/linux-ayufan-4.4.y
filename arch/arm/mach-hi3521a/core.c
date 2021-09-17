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
#include <linux/tags.h>
#include <mach/platform.h>

#include <asm/cacheflush.h>

void __iomem *hi3521a_gic_cpu_base_addr = IOMEM(CFG_GIC_CPU_BASE);

unsigned long long hi_sched_clock(void)
{
	return sched_clock();
}
EXPORT_SYMBOL(hi_sched_clock);

/*****************************************************************************/

void __init hi3521a_gic_init_irq(void)
{
	edb_trace();

#ifndef CONFIG_LOCAL_TIMERS
	gic_init(0, HISI_GIC_IRQ_START, IOMEM(CFG_GIC_DIST_BASE),
		 IOMEM(CFG_GIC_CPU_BASE));
#else
	/*
	 * git initialed include Local timer.
	 * IRQ_LOCALTIMER is settled IRQ number for local timer interrupt.
	 * It is set to 29 by ARM.
	 */
	gic_init(0, IRQ_LOCALTIMER, IOMEM(CFG_GIC_DIST_BASE),
		 IOMEM(CFG_GIC_CPU_BASE));
#endif
}

/*****************************************************************************/

static struct map_desc hi3521a_io_desc[] __initdata = {
	/* hi3521a_IOCH1 */
	{
		.virtual	= HI3521A_IOCH1_VIRT,
		.pfn		= __phys_to_pfn(HI3521A_IOCH1_PHYS),
		.length		= HI3521A_IOCH1_SIZE,
		.type		= MT_DEVICE
	},
	/* hi3521a_IOCH2 */
	{
		.virtual        = HI3521A_IOCH2_VIRT,
		.pfn            = __phys_to_pfn(HI3521A_IOCH2_PHYS),
		.length         = HI3521A_IOCH2_SIZE,
		.type           = MT_DEVICE
	},
	/* hi3521a_IOCH3 */
	{
		.virtual        = HI3521A_IOCH3_VIRT,
		.pfn            = __phys_to_pfn(HI3521A_IOCH3_PHYS),
		.length         = HI3521A_IOCH3_SIZE,
		.type           = MT_DEVICE
	},
};

/*****************************************************************************/

void __init hi3521a_map_io(void)
{
	int i;

	iotable_init(hi3521a_io_desc, ARRAY_SIZE(hi3521a_io_desc));

	edb_trace();
	for (i = 0; i < ARRAY_SIZE(hi3521a_io_desc); i++) {
		edb_putstr(" V: ");	edb_puthex(hi3521a_io_desc[i].virtual);
		edb_putstr(" P: ");	edb_puthex(hi3521a_io_desc[i].pfn);
		edb_putstr(" S: ");	edb_puthex(hi3521a_io_desc[i].length);
		edb_putstr(" T: ");	edb_putul(hi3521a_io_desc[i].type);
		edb_putstr("\n");
	}

	edb_trace();
}
/*****************************************************************************/

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
HIL_AMBA_DEVICE(uart2, "uart:2",  UART2,    NULL);

static struct amba_device *amba_devs[] __initdata = {
	&HIL_AMBADEV_NAME(uart0),
	&HIL_AMBADEV_NAME(uart1),
	&HIL_AMBADEV_NAME(uart2),
};

/*****************************************************************************/
/*
 * These are fixed clocks.
 */
static struct clk uart_clk = {
	.rate = 24000000,
};
static struct clk sp804_clk = {
	.rate	= 12500000, /* TODO: XXX */
};

static struct clk_lookup lookups[] = {
	{ /* UART0 */
		.dev_id		= "uart:0",
		.clk		= &uart_clk,
	}, { /* UART1 */
		.dev_id		= "uart:1",
		.clk		= &uart_clk,
	}, { /* UART2 */
		.dev_id		= "uart:2",
		.clk		= &uart_clk,
	},
	{ /* SP804 timers */
		.dev_id		= "sp804",
		.clk		= &sp804_clk,
	},
};
/*****************************************************************************/

static void __init hi3521a_reserve(void)
{
}
/*****************************************************************************/

void __init hi3521a_init(void)
{
	unsigned long i;

	edb_trace();
	for (i = 0; i < ARRAY_SIZE(amba_devs); i++) {
		edb_trace();
		amba_device_register(amba_devs[i], &iomem_resource);
	}
}
/*****************************************************************************/

static void __init hi3521a_init_early(void)
{
	unsigned long reg = 0;

	/* hi3521a ASIC uart use xtal osc clk 24M */
	reg = readl((void *)REG_PERI_CRG33);
	reg &= ~UART_CKSEL_MASK;
	reg |= UART_CKSEL_24M;
	writel(reg, (void *)REG_PERI_CRG33);

	sp804_clk.rate = get_bus_clk() / 4;

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
/*****************************************************************************/

void hi3521a_restart(char mode, const char *cmd)
{
	writel(~0, __io_address(REG_BASE_SCTL + REG_SC_SYSRES));
}
/*****************************************************************************/
extern void __init hi3521a_timer_init(void);

MACHINE_START(HI3521A, "bigfish")
	.atag_offset  = 0x100,
	.map_io       = hi3521a_map_io,
	.init_early   = hi3521a_init_early,
	.init_irq     = hi3521a_gic_init_irq,
	.init_time    = hi3521a_timer_init,
	.init_machine = hi3521a_init,
	.smp          = smp_ops(hi3521a_smp_ops),
	.reserve      = hi3521a_reserve,
	.restart      = hi3521a_restart,
MACHINE_END
