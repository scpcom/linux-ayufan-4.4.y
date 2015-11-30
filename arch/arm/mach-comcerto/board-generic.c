#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/sched_clock.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/printk.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>

#include <asm/hardware/cache-l2x0.h>

/* PFE */
#include <linux/phy.h>
#include <linux/clkdev.h>

#include <asm/mach/arch.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include "common.h"

static unsigned int persistent_mult, persistent_shift;
unsigned long axi_clk_rate;
static void __iomem *timer_base;


/* We need to explicitly enable certain clocks. Otherwise, they are considered
 * unused and clk_disable_unused() will disable them and freeze our system. */
static const char *clks_to_enable[] = {
	"arm_clk",
	"axi_clk",
	"ddr_clk",
	"l2cc_clk",

	"hfe_core_clk", /* PFE */

	"sata_oob_clk", /* SATA */
	"sata_pmu_clk", /* SATA */
	NULL
};
static int __init c2k_enable_clks(void)
{
	const char **c;
	struct device_node *node;
	struct clk *clk;
	struct of_phandle_args clkspec;
	for (c = clks_to_enable; *c != NULL; c++) {
		node = of_find_node_by_name(NULL, *c);
		clkspec.np = node;
		clkspec.args_count = 0;
		clk = of_clk_get_from_provider(&clkspec);

		if (!IS_ERR(clk)) {
			clk_prepare(clk);
			clk_enable(clk);
		} else {
			pr_warn("failed to lookup clock node %s\n",
				*c);
		}
	}
	return 0;


	/* Set the SATA PMU clock to 30 MHZ and OOB clock to 125MHZ */
	node = of_find_node_by_name(NULL, "sata_oob_clk");
	clkspec.np = node;
	clkspec.args_count = 0;
	clk = of_clk_get_from_provider(&clkspec);

	if (!IS_ERR(clk)) {
		clk_set_rate(clk,125000000);
	} else {
		BUG();
	}
	node = of_find_node_by_name(NULL, "sata_pmu_clk");
	clkspec.np = node;
	clkspec.args_count = 0;
	clk = of_clk_get_from_provider(&clkspec);

	if (!IS_ERR(clk)) {
		clk_set_rate(clk,30000000);
	} else {
		BUG();
	}

}

late_initcall(c2k_enable_clks);

struct clk_lookup axi_lk = {
	.dev_id = NULL,
	.con_id = "axi",
};

struct clk_lookup gemtx_lk = {
	.dev_id = NULL,
	.con_id = "gemtx",
};

static int __init c2k_register_clks(void)
{
	struct device_node *node;
	struct clk *clk;
	struct of_phandle_args clkspec;
	node = of_find_node_by_name(NULL, "axi_clk");
	clkspec.np = node;
	clkspec.args_count = 0;
	clk = of_clk_get_from_provider(&clkspec);
	if (!IS_ERR(clk)) {
		axi_lk.clk = clk;
		clkdev_add(&axi_lk);
	} else {
		pr_warn("failed to lookup clock node axi_clk\n");
	}


	node = of_find_node_by_name(NULL, "gemtx_clk");
	clkspec.np = node;
	clkspec.args_count = 0;
	clk = of_clk_get_from_provider(&clkspec);
	if (!IS_ERR(clk)) {
		gemtx_lk.clk = clk;
		clkdev_add(&gemtx_lk);
	} else {
		pr_warn("failed to lookup clock node gemtx_clk\n");
	}
	return 0;
}

late_initcall(c2k_register_clks);

static int c2k_timer_set_next_event(unsigned long cycles,
					 struct clock_event_device *evt);
static void c2k_timer_set_mode(enum clock_event_mode mode,
				    struct clock_event_device *evt);

static struct clock_event_device clockevent_c2k = {
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT, // TODO
	.rating		= 300,
	.set_next_event	= c2k_timer_set_next_event,
	.set_mode	= c2k_timer_set_mode,
	.name		= "timer1",
};

#define timer_mask(timer)	(1 << timer)

/*
 * Routine to catch timer interrupts
 */
static irqreturn_t comcerto_timer1_interrupt(int irq, void *dev_id)
{
	u32 status;
	struct clock_event_device *dev = &clockevent_c2k;

	status = __raw_readl(timer_base + COMCERTO_TIMER_STATUS) & __raw_readl(timer_base + COMCERTO_TIMER_IRQ_MASK);

	/* timer1 expired */
	if (status & timer_mask(1)) {
		/* we need to disable interrupt to simulate ONESHOT mode,
		   do it before clearing the interrupt to avoid race */
		if (dev->mode != CLOCK_EVT_MODE_PERIODIC)
			__raw_writel(__raw_readl(timer_base + COMCERTO_TIMER_IRQ_MASK) & ~(1 << (1)), timer_base + COMCERTO_TIMER_IRQ_MASK);

		__raw_writel(1 << (1), timer_base + COMCERTO_TIMER_STATUS_CLR);
		dev->event_handler(dev);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static struct irqaction comcerto_timer1_irq = {
	.name		= "timer1",
	.flags		= IRQF_TIMER,
	.handler	= comcerto_timer1_interrupt,
};


static int c2k_timer_set_next_event(unsigned long cycles,
					 struct clock_event_device *evt)
{
	pr_err("%s cycles %lu\n", __func__, cycles);
	/* now write correct bound and clear interrupt status.
	   Writing high bound register automatically resets count to low bound value.
	   For very small bound values it's possible that we ack the interrupt _after_ the timer has already expired,
	   this is not very serious because the interrupt will be asserted again in a very short time */
	__raw_writel(cycles & 0x3FFFFFFF, timer_base + COMCERTO_TIMER1_HIGH_BOUND);
	__raw_writel(1 << 1, timer_base + COMCERTO_TIMER_STATUS_CLR);

	/* enable interrupt for ONESHOT mode */
	if (evt->mode == CLOCK_EVT_MODE_ONESHOT)
		__raw_writel(__raw_readl(timer_base + COMCERTO_TIMER_IRQ_MASK) | (1 << 1), timer_base + COMCERTO_TIMER_IRQ_MASK);

	return 0;
}

static void c2k_timer_set_mode(enum clock_event_mode mode,
				    struct clock_event_device *evt)
{
	u32 period;
	pr_err("%s mode %d\n", __func__, mode);

	__raw_writel(__raw_readl(timer_base + COMCERTO_TIMER_IRQ_MASK) & ~(1 << (1)), timer_base + COMCERTO_TIMER_IRQ_MASK);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		period = axi_clk_rate / HZ;
		period -= 1;
		__raw_writel(period & 0x3FFFFFFF, timer_base + COMCERTO_TIMER1_HIGH_BOUND);
		__raw_writel(__raw_readl(timer_base + COMCERTO_TIMER_IRQ_MASK) | (1 << (1)), timer_base + COMCERTO_TIMER_IRQ_MASK);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static void __init c2k_clockevent_init(int timer1_irq) {
	/* Clear all the timers except timer0  */
	__raw_writel(COMCERTO_TIMER_CSP, timer_base + COMCERTO_TIMER_STATUS);

	/* Register interrupt handler for interrupt on IRQ_TIMERB*/
	irq_set_irq_type(timer1_irq, IRQ_TYPE_EDGE_RISING);
	setup_irq(timer1_irq, &comcerto_timer1_irq);
	clockevent_c2k.cpumask = cpu_possible_mask;
	clockevent_c2k.irq = timer1_irq;
	clockevents_config_and_register(&clockevent_c2k, axi_clk_rate,
					1,
					0x3fffffff);
}


static u64 notrace c2k_read_sched_clock(void)
{
	return readl_relaxed(timer_base + COMCERTO_TIMER2_CURRENT_COUNT);
}

static void __init _ls1024a_timer_init(int timer1_irq, struct clk *clk_axi)
{
	int ret;

	axi_clk_rate = clk_get_rate(clk_axi);
	/*
	err = twd_local_timer_register(&twd_local_timer);
	if (err)
		pr_err("twd_local_timer_register failed %d\n", err);
		*/
	__raw_writel(0, timer_base + COMCERTO_TIMER2_CTRL);
	__raw_writel(0, timer_base + COMCERTO_TIMER2_LOW_BOUND);
	__raw_writel(0xffffffff, timer_base + COMCERTO_TIMER2_HIGH_BOUND);
	ret = clocksource_mmio_init(timer_base + COMCERTO_TIMER2_CURRENT_COUNT, "timer2", axi_clk_rate,
			250, 32, clocksource_mmio_readl_up);
	clocks_calc_mult_shift(&persistent_mult, &persistent_shift,
			axi_clk_rate, NSEC_PER_SEC, 120000);
	sched_clock_register(c2k_read_sched_clock, 32, axi_clk_rate);
	c2k_clockevent_init(timer1_irq);
}

static void __init ls1024a_timer_init_dt(struct device_node *np)
{
	struct clk *clk_axi;
	int irq;

	timer_base = of_iomap(np, 0);
	WARN_ON(!timer_base);
	irq = irq_of_parse_and_map(np, 1);

	clk_axi = of_clk_get(np, 0);
	if (IS_ERR(clk_axi)) {
		pr_err("LS1024A timer: unable to get AXI clk\n");
		return;
	}

	_ls1024a_timer_init(irq, clk_axi);
}

CLOCKSOURCE_OF_DECLARE(ls1024a_timer, "fsl,ls1024a-gpt", ls1024a_timer_init_dt);


static struct resource comcerto_pfe_resources[] = {
	{
		.name	= "apb",
		.start  = COMCERTO_APB_PFE_BASE,
		.end    = COMCERTO_APB_PFE_BASE + COMCERTO_APB_PFE_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name	= "axi",
		.start  = COMCERTO_AXI_PFE_BASE,
		.end    = COMCERTO_AXI_PFE_BASE + COMCERTO_AXI_PFE_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name	= "ddr",
		.start  = COMCERTO_PFE_DDR_BASE,
		.end	= COMCERTO_PFE_DDR_BASE + COMCERTO_PFE_DDR_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name	= "iram",
		.start  = COMCERTO_PFE_IRAM_BASE,
		.end	= COMCERTO_PFE_IRAM_BASE + COMCERTO_PFE_IRAM_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
        {
                .name   = "ipsec",
                .start  = COMCERTO_AXI_IPSEC_BASE,
                .end    = COMCERTO_AXI_IPSEC_BASE + COMCERTO_AXI_IPSEC_SIZE - 1,
                .flags  = IORESOURCE_MEM,
        },

};

static struct comcerto_pfe_platform_data optimus_pfe_pdata = {
	.comcerto_eth_pdata[0] = {
		.name = "lan0",
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_NO_PHY,
		.gem_id = 0,
		.mac_addr = (u8[])GEM0_MAC,
	},

	.comcerto_eth_pdata[1] = {
		.name = "wan0",
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_PHY_RGMII_ADD_DELAY,
		.bus_id = 0,
		.phy_id = 4,
		.gem_id = 1,
		.mac_addr = (u8[])GEM1_MAC,
	},

	.comcerto_eth_pdata[2] = {
		.name = "moca0",
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_NO_PHY,
		.gem_id = 2,
		.mac_addr = (u8[])GEM2_MAC,
	},

	/**
	 * There is a single mdio bus coming out of C2K.  And that's the one
	 * connected to GEM0. All PHY's, switchs will be connected to the same
	 * bus using different addresses. Typically .bus_id is always 0, only
	 * .phy_id will change in the different comcerto_eth_pdata[] structures above.
	 */
	.comcerto_mdio_pdata[0] = {
		.enabled = 1,
		.phy_mask = 0xFFFFFFEF,
		.mdc_div = 96,
		.irq = {
			[4] = PHY_POLL,
		},
	},
};

static struct comcerto_pfe_platform_data gfsc100_pfe_pdata = {
	.comcerto_eth_pdata[0] = {
		.name = "unused",
		.phy_flags = GEMAC_NO_PHY,
		.gem_id = 0,
	},

	.comcerto_eth_pdata[1] = {
		.name = "lan0",
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_PHY_RGMII_ADD_DELAY,
		.bus_id = 0,
		.phy_id = 1,
		.gem_id = 1,
		.mac_addr = (u8[])GEM1_MAC,
	},

	.comcerto_eth_pdata[2] = {
		.name = "unused",
		.phy_flags = GEMAC_NO_PHY,
		.gem_id = 2,
	},

	/**
	 * There is a single mdio bus coming out of C2K.  And that's the one
	 * connected to GEM0. All PHY's, switchs will be connected to the same
	 * bus using different addresses. Typically .bus_id is always 0, only
	 * .phy_id will change in the different comcerto_eth_pdata[] structures above.
	 */
	.comcerto_mdio_pdata[0] = {
		.enabled = 1,
		.phy_mask = 0xFFFFFFFD,
		.mdc_div = 96,
		.irq = {
			[1] = PHY_POLL,
		},
	},
};

static u64 comcerto_pfe_dma_mask = DMA_BIT_MASK(32);

static struct platform_device comcerto_pfe_device = {
	.name		= "pfe",
	.id		= 0,
	.dev		= {
		.dma_mask		= &comcerto_pfe_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(comcerto_pfe_resources),
	.resource	= comcerto_pfe_resources,
};

static int is_mac_zero(u8 *buf)
{
        unsigned long dm;
        for (dm = 0; dm < 6; dm++){
		if ((*(buf+dm)) != 0)
			return 1;
	}
	return 0;
}

static int __init mac_addr_atoi(u8 mac_addr[], char *mac_addr_str)
{
	int i, j, k;
	int str_incr_cnt = 0;

	if (*mac_addr_str == ',') {
		mac_addr_str++;
		str_incr_cnt++;
		return str_incr_cnt;
	}

	for (i = 0; i < 6; i++) {

		j = hex_to_bin(*mac_addr_str++);
		str_incr_cnt++;
		if (j == -1)
			return str_incr_cnt;

		k = hex_to_bin(*mac_addr_str++);
		str_incr_cnt++;
		if (k == -1)
			return str_incr_cnt;

		mac_addr_str++;
		str_incr_cnt++;
		mac_addr[i] = (j << 4) + k;
	}

	return str_incr_cnt;
}

static u8 c2k_mac_addr[3][14];

static int __init mac_addr_setup(char *str)
{
	int str_incr_cnt = 0;

	if (*str++ != '=' || !*str)  /* No mac addr specified */
		return -1;

	str_incr_cnt = mac_addr_atoi(c2k_mac_addr[0], str);

	str += str_incr_cnt;

	str_incr_cnt = mac_addr_atoi(c2k_mac_addr[1], str);

	str += str_incr_cnt;

	mac_addr_atoi(c2k_mac_addr[2], str);

	return 0;
}
__setup("mac_addr", mac_addr_setup);

void __init mac_addr_init(struct comcerto_pfe_platform_data * comcerto_pfe_data_ptr)
{
	u8 gem_port_id;

	for (gem_port_id = 0; gem_port_id < NUM_GEMAC_SUPPORT; gem_port_id++) {
		if (is_mac_zero(c2k_mac_addr[gem_port_id]))  /* If mac is non-zero */
			comcerto_pfe_data_ptr->comcerto_eth_pdata[gem_port_id].mac_addr = c2k_mac_addr[gem_port_id];
	}

}

static void __init pfe_init_pdata(void) {
	if (of_machine_is_compatible("google,gfsc100")) {
		comcerto_pfe_device.dev.platform_data = &gfsc100_pfe_pdata;
	} else {
		comcerto_pfe_device.dev.platform_data = &optimus_pfe_pdata;
	}
}

static int __init register_pfe_device(void) {
	int rc;
	pfe_init_pdata();
	mac_addr_init(comcerto_pfe_device.dev.platform_data);
	rc = platform_device_register(&comcerto_pfe_device);
	return 0;
}
late_initcall(register_pfe_device);


static const char * const c2k_boards_compat[] __initconst = {
	"fsl,ls1024a",
	NULL,
};

DT_MACHINE_START(C2K_DT, "Generic Freescale LS1024A (Flattened Device Tree)")
	.l2c_aux_val	= L2C_AUX_CTRL_SHARED_OVERRIDE | (1<<L220_AUX_CTRL_FWA_SHIFT), // 0,
	.l2c_aux_mask	= ~( L2C_AUX_CTRL_SHARED_OVERRIDE | L220_AUX_CTRL_FWA_MASK ), //~0,
	.reserve	= c2k_reserve,
	.map_io		= c2k_map_io,
	.init_early	= c2k_init_early,
	.init_late	= c2k_init_late,
	.dt_compat	= c2k_boards_compat,
MACHINE_END
