/*
 *  linux/arch/arm/mach-comcerto/comcerto-2000.c
 *
 *  Copyright (C) 2011 Mindspeed Technologies, Inc.
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

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/antirebootloop.h>

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/irqs.h>
#include <linux/delay.h>
#include <asm/pmu.h>
#include <mach/dma.h>
#include <linux/dma-mapping.h>

#include <linux/serial_8250.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/smp.h>
#include <linux/uio_driver.h>

#include <asm/mach-types.h>
#include <asm/smp_twd.h>
#include <asm/localtimer.h>

#include <asm/mach/time.h>
#include <asm/hardware/gic.h>
#include <mach/reset.h>

#include <mach/hardware.h>
#include <asm/hardware/cache-l2x0.h>
#include <mach/comcerto-2000.h>
#include <linux/ahci_platform.h>
#include <mach/serdes-c2000.h>
#include <linux/clockchips.h>
#include <linux/clk.h>
#include <mach/comcerto-2000/clock.h>
#include <mach/comcerto-2000/pm.h>
#include <mach/gpio.h>

#include <linux/module.h>

struct c2k_gpio_pin_stat_info c2k_gpio_pin_stat =
{
	.c2k_gpio_pins_0_31 = 0x0,
	.c2k_gpio_pins_32_63 = 0x0,
};

#ifdef CONFIG_CACHE_L2X0
void __iomem *l2cache_base;
#endif

/***********************************************************
 *   Virtual address Mapping                               *
 *                                                         *
 ***********************************************************/

static struct map_desc comcerto_io_desc[] __initdata =
{
#ifdef CONFIG_COMCERTO_MSP
	{
		.virtual    = COMCERTO_MSP_VADDR,
		.pfn        = __phys_to_pfn(COMCERTO_MSP_DDR_BASE),
		.length     = COMCERTO_MSP_DDR_SIZE_CB,
		.type       = MT_MSP
	},
	{
		.virtual    = (COMCERTO_MSP_VADDR + COMCERTO_MSP_DDR_SIZE_CB),
		.pfn        = __phys_to_pfn(COMCERTO_MSP_DDR_BASE + COMCERTO_MSP_DDR_SIZE_CB),
		.length     = COMCERTO_MSP_DDR_SIZE_NCNB,
		.type       = MT_MSP_NCNB
	},
       {
               .virtual    = COMCERTO_PFE_VADDR,
               .pfn        = __phys_to_pfn(COMCERTO_PFE_DDR_BASE),
               .length     = COMCERTO_PFE_DDR_SIZE,
               .type       = MT_DEVICE
       },
       {
               .virtual    = COMCERTO_PFE_AXI_VADDR,
               .pfn        = __phys_to_pfn(COMCERTO_AXI_PFE_BASE),
               .length     = COMCERTO_AXI_PFE_SIZE,
               .type       = MT_DEVICE
       },
#endif  /* CONFIG_COMCERTO_MSP */
#if defined(CONFIG_PCI)
        {
                .virtual    = COMCERTO_AXI_PCIe0_VADDR_BASE,
                .pfn        = __phys_to_pfn(COMCERTO_AXI_PCIe0_BASE),
                .length     = SZ_16M,
                .type       = MT_DEVICE
        },
        {
                .virtual    = COMCERTO_AXI_PCIe1_VADDR_BASE,
                .pfn        = __phys_to_pfn(COMCERTO_AXI_PCIe1_BASE),
                .length     = SZ_16M,
                .type       = MT_DEVICE
        },
#endif
	{
		.virtual    = COMCERTO_SCU_VADDR,
		.pfn        = __phys_to_pfn(COMCERTO_SCU_BASE),
		.length     = SZ_128K,
		.type       = MT_DEVICE
	},
	{
		.virtual    = IRAM_MEMORY_VADDR,
		.pfn        = __phys_to_pfn(COMCERTO_AXI_IRAM_BASE),
		.length     = IRAM_MEMORY_SIZE,
		.type       = MT_DEVICE
	},
	{
		.virtual    = COMCERTO_APB_VADDR,
		.pfn        = __phys_to_pfn(COMCERTO_AXI_APB_BASE),
		.length     = COMCERTO_APB_SIZE,
		.type       = MT_DEVICE
	},
	{
		.virtual	= COMCERTO_AXI_UART_SPI_VADDR,
		.pfn		= __phys_to_pfn(COMCERTO_AXI_UART_SPI_BASE),
		.length 	= COMCERTO_AXI_UART_SPI_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= COMCERTO_DECT_VADDR,
		.pfn		= __phys_to_pfn(COMCERTO_AXI_DECT_BASE),
		.length		= 2 * SZ_1M,
		.type		= MT_DEVICE
	},
	{
		.virtual    = COMCERTO_SEMA_VADDR,
		.pfn        = __phys_to_pfn(COMCERTO_AXI_SEMA_BASE),
		.length     = SZ_16M,
		.type       = MT_DEVICE
	},
};

#if defined(CONFIG_COMCERTO_64K_PAGES)
#define PFE_DMA_SIZE		(4 * SZ_1M)
#else
#define PFE_DMA_SIZE            (16 * SZ_1M)
#endif

#define DSPG_DECT_CSS_DMA_SIZE	(10 * SZ_1M)

void __init device_map_io(void)
{
	unsigned long size = PFE_DMA_SIZE;

#if defined(CONFIG_DSPG_DECT_CSS)
	size += DSPG_DECT_CSS_DMA_SIZE;
#endif

	iotable_init(comcerto_io_desc, ARRAY_SIZE(comcerto_io_desc));

	/* Increase consistent DMA zone size */
	init_consistent_dma_size(size);
}


void __init device_irq_init(void)
{
	gic_init(0, SGI_IRQ(1), (void *)COMCERTO_GIC_DIST_VADDR, (void *)COMCERTO_GIC_CPU_VADDR);
}

/************************************************************************
 *  GPIO
 ************************************************************************/
static __init void gpio_init(void)
{

#if defined(CONFIG_COMCERTO_UART1_SUPPORT)
	writel(readl(COMCERTO_GPIO_MISC_PIN_SELECT) & ~0x3, COMCERTO_GPIO_MISC_PIN_SELECT);
#endif

#if defined(CONFIG_COMCERTO_UART0_SUPPORT)
	writel((readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~UART0_GPIO) | UART0_BUS, COMCERTO_GPIO_PIN_SELECT_REG);
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= UART0_GPIO_PIN; /* GPIOs 8 to 11 are used for UART0 */
#endif

	/*
	 * Configure each GPIO to be Output or Input
	 * When Input, Configure to be Input, IRQ
	 * When Input IRQ, Configure to be IRQ Rising Edge or IRQ falling Edge
	 */

	/*[FIXME]: GPIO Output, others are input*/
	__raw_writel(__raw_readl(COMCERTO_GPIO_OE_REG) | COMCERTO_OUTPUT_GPIO, COMCERTO_GPIO_OE_REG);

	/*
	 * Default GPIO IRQ Configuration. Enable specific IRQs in the
	 * board-*.c file
	 */
	__raw_writel(0, COMCERTO_GPIO_INT_CFG_REG);

	/* [FIXME]: Are pins GPIO or pins used by another block*/
	//__raw_writel(COMCERTO_GPIO_PIN_USAGE, COMCERTO_GPIO_IOCTRL_REG);
}

/************************************************************************
 *  Expansion Bus
 ************************************************************************/

/*This variable is provided by the board file*/
extern int comcerto_exp_values[5][7];

static __init void exp_bus_init(void)
{
	int cs;
	u32 cs_enable;
	unsigned int axi_clk, clk_div;
	struct clk *clk_axi;

	/*First, Reset the Expansion block*/
	__raw_writel(0x1, COMCERTO_EXP_SW_RST_R);

	while (readl(COMCERTO_EXP_SW_RST_R) & 0x1) ;

	/* Clock divider configuration, get the AXI clock first
	 * AXI clock will be used for refernce count , as exp bus
         * also have a dependancy with AXI.
	*/
        clk_axi = clk_get(NULL,"axi");

	if (IS_ERR(clk_axi)) {
		pr_err("comcerto_Device_init: Unable to obtain axi clock: %ld\n",PTR_ERR(clk_axi));
	}

	/*Enable the AXI clock */
	if (clk_enable(clk_axi)){
		pr_err("%s: Unable to enable axi clock:\n",__func__);
	}

        /* Get the AXI clock rate in HZ */
        axi_clk = clk_get_rate(clk_axi);
	/* Round divider up */
	clk_div = (axi_clk + COMCERTO_EXPCLK - 1) / COMCERTO_EXPCLK;

	__raw_writel(clk_div, COMCERTO_EXP_CLOCK_DIV_R);

	cs_enable = 0;
	for (cs = 0; cs < 5; cs++) {
		/*configure only enabled CS */
		{
			if (comcerto_exp_values[cs][0] == 1)
				cs_enable |= EXP_CSx_EN(cs);

			/*mode configuration*/
			__raw_writel(comcerto_exp_values[cs][3], COMCERTO_EXP_CSx_CFG_R(cs));

			/*Chip select Base configuration (start of address space)*/
			__raw_writel(comcerto_exp_values[cs][1], COMCERTO_EXP_CSx_BASE_R(cs));

			/*Chip select Segment size configuration (end of address space)*/
			__raw_writel(comcerto_exp_values[cs][2], COMCERTO_EXP_CSx_SEG_R(cs));

			/*Chip select timing configuration*/
			__raw_writel(comcerto_exp_values[cs][4], COMCERTO_EXP_CSx_TMG1_R(cs));
			__raw_writel(comcerto_exp_values[cs][5], COMCERTO_EXP_CSx_TMG2_R(cs));
			__raw_writel(comcerto_exp_values[cs][6], COMCERTO_EXP_CSx_TMG3_R(cs));
		}
	}

	/*Chip Select activation*/
	__raw_writel(EXP_CLK_EN | cs_enable, COMCERTO_EXP_CS_EN_R);
}

static u32 armv7_aux_ctrl_read(void)
{
	u32 val;

	asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r"(val));

	printk(KERN_INFO "ARMv7 AUX CTRL(%d): %#x\n", smp_processor_id(), val);

	return val;
}

static void armv7_aux_ctrl_write(u32 val)
{
	printk(KERN_INFO "ARMv7 AUX CTRL(%d): %#x\n", smp_processor_id(), val);
	asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r"(val));
}

static void armv7_aux_ctrl_setup(void *info)
{
	u32 aux_ctrl;
	unsigned long flags;

	local_irq_save(flags);

	aux_ctrl = armv7_aux_ctrl_read();

#ifdef CONFIG_PL310_FULL_LINE_OF_ZERO
	aux_ctrl |= (1 << 3);
#endif

#ifdef CONFIG_PL310_EXCLUSIVE_CACHE
	aux_ctrl |= (1 << 7);
#endif

	armv7_aux_ctrl_write(aux_ctrl);

	local_irq_restore(flags);
}

#ifdef CONFIG_CACHE_L2X0
void l2x0_latency(u32 tag_ram_setup_lat, u32 tag_ram_rd_lat, u32 tag_ram_wr_lat, u32 data_ram_setup_lat, u32 data_ram_rd_lat, u32 data_ram_wr_lat)
{
	u32 val;

	val = ((tag_ram_wr_lat - 1) << COMCERTO_L2CC_WR_LAT_SHIFT) | ((tag_ram_rd_lat - 1) << COMCERTO_L2CC_RD_LAT_SHIFT) | (tag_ram_setup_lat - 1);
	writel(val, l2cache_base + L2X0_TAG_LATENCY_CTRL);

	val = ((data_ram_wr_lat - 1) << COMCERTO_L2CC_WR_LAT_SHIFT) | ((data_ram_rd_lat - 1) << COMCERTO_L2CC_RD_LAT_SHIFT) | (data_ram_setup_lat - 1);
	writel(val, l2cache_base + L2X0_DATA_LATENCY_CTRL);
}

void comcerto_l2cc_init(void)
{
	unsigned int aux_val, aux_mask;
	unsigned int associativity, waysize;
#ifdef CONFIG_L2X0_INSTRUCTION_ONLY
	int i;
#endif


	l2cache_base = (void *)COMCERTO_L310_VADDR;
	BUG_ON(!l2cache_base);

	/* Set Latency of L2CC to minimum (i.e. 1 cycle) */
	l2x0_latency(1, 1, 1, 1, 1, 1);

	/* Set L2 address filtering, use L2CC M1 port for DDR accesses */
	writel(0x80000000, l2cache_base + L2X0_ADDR_FILTER_END);
	writel(0x00000000 | L2X0_ADDR_FILTER_EN, l2cache_base + L2X0_ADDR_FILTER_START);

	associativity = (COMCERTO_L2CC_ASSOCIATIVITY_8WAY << COMCERTO_L2CC_ASSOCIATIVITY_SHIFT) & COMCERTO_L2CC_ASSOCIATIVITY_MASK;
	waysize = (COMCERTO_L2CC_ASSOCIATIVITY_32KB << COMCERTO_L2CC_WAYSIZE_SHIFT) & COMCERTO_L2CC_WAYSIZE_MASK;
	aux_val = associativity | waysize;
	aux_mask = (COMCERTO_L2CC_ASSOCIATIVITY_MASK | COMCERTO_L2CC_WAYSIZE_MASK);

	/* Shareable attribute override enable */
	/* This prevents the cache from changing "normal memory/non-cacheable" accesses to
	"normal memory/cacheable/writethrough no read/write allocate"*/
	aux_val |= (1 << 22);
	aux_mask |= (1 << 22);

	/* Write allocate override, no write allocate */
	aux_val |= (1 << 23);
	aux_mask |= (3 << 23);

#ifdef CONFIG_PL310_FULL_LINE_OF_ZERO
	aux_val |= (1 << 0);
	aux_mask |= (1 << 0);
#endif

#ifdef CONFIG_PL310_EARLY_WRITE_RESPONSE
	aux_val |= (1 << 30);
	aux_mask |= (1 << 30);
#endif

#ifdef CONFIG_PL310_STORE_BUFFER_DEVICE_LIMITATION
	aux_val |= (1 << 11);
	aux_mask |= (1 << 11);
#endif

#ifdef CONFIG_PL310_INSTRUCTION_PREFETCH
	aux_val |= (1 << 29);
	aux_mask |= (1 << 29);
#endif

#ifdef CONFIG_PL310_DATA_PREFETCH
	aux_val |= (1 << 28);
	aux_mask |= (1 << 28);
#endif


#ifdef CONFIG_PL310_EXCLUSIVE_CACHE
	aux_val |= (1 << 12);
	aux_mask |= (1 << 12);
#endif

	/* L2 8-way associativity with 32KB way size */
	l2x0_init(l2cache_base, aux_val, aux_mask);

#ifdef CONFIG_L2X0_INSTRUCTION_ONLY
	for (i = 0; i < 8; i++)
		writel_relaxed(0xffff, l2cache_base + L2X0_LOCKDOWN_WAY_D_BASE + i * L2X0_LOCKDOWN_STRIDE);

	outer_flush_all();
#endif

	armv7_aux_ctrl_setup(NULL);

	smp_call_function(armv7_aux_ctrl_setup, NULL, 1);
}
#endif

#if defined(CONFIG_COMCERTO_SATA)

#define SERDES_PHY1     1
#define SERDES_PHY2     2

static int comcerto_ahci_init(struct device *dev, void __iomem *mmio)
{
	struct serdes_regs_s *p_sata_phy_reg_file;
	int serdes_regs_size;
        u32 val;
	int ref_clk_24;

	/* Move SATA controller to DDRC2 port */
	writel(readl(COMCERTO_GPIO_FABRIC_CTRL_REG) | 0x2, COMCERTO_GPIO_FABRIC_CTRL_REG);

	val = readl(COMCERTO_GPIO_SYSTEM_CONFIG);
	ref_clk_24 = val & (BIT_5_MSK|BIT_7_MSK);

	if(ref_clk_24)
	{
		p_sata_phy_reg_file = &sata_phy_reg_file_24[0];
		serdes_regs_size = sizeof(sata_phy_reg_file_24);
		printk(KERN_INFO "SATA Serdes: 24Mhz ref clk\n");
	}
	else
	{
		p_sata_phy_reg_file = &sata_phy_reg_file_48[0];
		serdes_regs_size = sizeof(sata_phy_reg_file_48);
		printk(KERN_INFO "SATA Serdes: 48Mhz ref clk\n");
	}

	//Take SATA AXI domain out of reset
	c2000_block_reset(COMPONENT_AXI_SATA,0);
	//Bring SATA PMU and OOB out of reset
	c2000_block_reset(COMPONENT_SATA_PMU,0);
	c2000_block_reset(COMPONENT_SATA_OOB,0);

        if ( (val & BOOT_SERDES1_CNF_SATA0) || (!(val & BOOT_SERDES2_CNF_SATA1)))
        {
                if (val & BOOT_SERDES1_CNF_SATA0)
                {
			//Bring Serdes1 out of reset
			c2000_block_reset(COMPONENT_SERDES1,0);
			//Bring SATA0 out of reset
			c2000_block_reset(COMPONENT_SERDES_SATA0,0);

                        /* Serdes Initialization. */
                        if( serdes_phy_init(SERDES_PHY1,  p_sata_phy_reg_file,
                                                serdes_regs_size / sizeof(serdes_regs_t),
                                                SD_DEV_TYPE_SATA) )
                        {
                                printk(KERN_ERR "%s: Failed to initialize serdes1 !!\n", __func__);
                                return -1;
                        }

                }

                if (!(val & BOOT_SERDES2_CNF_SATA1))
                {
			//Bring Serdes2 out of reset
			c2000_block_reset(COMPONENT_SERDES2,0);
			//Bring SATA1 out of reset
			c2000_block_reset(COMPONENT_SERDES_SATA1,0);

                        /* Serdes Initialization. */
                        if( serdes_phy_init(SERDES_PHY2,  p_sata_phy_reg_file,
                                                serdes_regs_size / sizeof(serdes_regs_t),
                                                SD_DEV_TYPE_SATA) )
                        {
                                printk(KERN_ERR "%s: Failed to initialize serdes2 !!\n", __func__);
                                return -1;
                        }
                }
        } else
                return -1;

        return 0;
}
#endif

#if defined(CONFIG_COMCERTO_UART0_SUPPORT) || defined(CONFIG_COMCERTO_UART1_SUPPORT)
#define UART_DWC_USR	0x1F
static int fastuart_handle_irq(struct uart_port *p)
{
	unsigned int iir = p->serial_in(p, UART_IIR);
	unsigned int dummy;
	if (serial8250_handle_irq(p, iir)) {
		return 1;
	} else if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
		/* Clear the USR */
		dummy = p->serial_in(p, UART_DWC_USR);
		return 1;
	}

	return 0;
}
#endif

static struct resource comcerto_pmu_resources[] = {
	{
		.start	= IRQ_A9_PMU0,
		.end	= IRQ_A9_PMU0,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_A9_PMU1,
		.end	= IRQ_A9_PMU1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device comcerto_pmu = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.num_resources = ARRAY_SIZE(comcerto_pmu_resources),
	.resource = comcerto_pmu_resources,
};

/* --------------------------------------------------------------------
 *  Serial interface
 * -------------------------------------------------------------------- */
#if defined(CONFIG_COMCERTO_UART0_SUPPORT) || defined(CONFIG_COMCERTO_UART1_SUPPORT)
static struct plat_serial8250_port comcerto_uart_data[] = {
#ifdef CONFIG_COMCERTO_UART1_SUPPORT
        {
                .mapbase        = COMCERTO_AXI_UART1_BASE,
		.membase	= (void *)COMCERTO_AXI_UART1_VADDR,
                .irq            = IRQ_UART1,
		.handle_irq	= fastuart_handle_irq,
                .flags          = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_IOREMAP,
                .iotype         = UPIO_MEM,
                .regshift       = 2,
        },
#endif
#ifdef CONFIG_COMCERTO_UART0_SUPPORT
        {
                .mapbase        = COMCERTO_AXI_UART0_BASE,
                .membase        = (void *)COMCERTO_AXI_UART0_VADDR,
                .irq            = IRQ_UART0,
		.handle_irq	= fastuart_handle_irq,
                .flags          = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_IOREMAP,
                .iotype         = UPIO_MEM,
                .regshift       = 2,
	},
#endif
	{
		.flags          = 0,
	},
};

static struct platform_device comcerto_uart = {
	.name       = "serial8250",
	.id         = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data	= comcerto_uart_data,
	},
};
#endif

#ifdef CONFIG_COMCERTO_MSP
static struct resource comcerto_ved_resources[] = {
	{
		.name  = "voip",
		.start = COMCERTO_MSP_DDR_BASE,
		.end   = COMCERTO_MSP_DDR_BASE + COMCERTO_MSP_DDR_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "irq",
		.start = IRQ_PTP0,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device comcerto_ved = {
	.name = "ved",
	.id = 0,
	.num_resources = ARRAY_SIZE(comcerto_ved_resources),
	.resource = comcerto_ved_resources,
};
#endif  /* CONFIG_COMCERTO_MSP */


/* --------------------------------------------------------------------
 *  USB 3.0 Host
 * -------------------------------------------------------------------- */

#if defined(CONFIG_COMCERTO_USB3_SUPPORT)
static struct resource comcerto_usb3_resource[] = {
        [0] = {
                .start  = COMCERTO_AXI_USB3P0_BASE,
                .end    = COMCERTO_AXI_USB3P0_BASE + SZ_8M - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = IRQ_USB3,
                .end    = IRQ_USB3,
                .flags  = IORESOURCE_IRQ,
        },
};

static u64 comcerto_usb3_dmamask = 0xfffff000;

struct platform_device comcerto_device_usb3 = {
        .name           = "xhci-hcd",
        .id             = -1,
        .resource       = comcerto_usb3_resource,
        .num_resources  = ARRAY_SIZE(comcerto_usb3_resource),
        .dev            = {
                .dma_mask               = &comcerto_usb3_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
        },
};
#endif

/* --------------------------------------------------------------------
 *  USB 2.0 Host 
 * -------------------------------------------------------------------- */

#if defined(CONFIG_COMCERTO_USB2_SUPPORT)

static u64 comcerto_dwc_otg_dmamask = 0xFFFFFFFF /*DMA_BIT_MASK(32)*/;

static struct resource comcerto_dwc_otg_resources[] = {
	{
		.start	= COMCERTO_AXI_USB2P0_BASE,
		.end	= COMCERTO_AXI_USB2P0_BASE + 0xFFFFFF,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_USB2,
		.end	= IRQ_USB2,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device comcerto_dwc_otg_device = {
	.name			= "dwc_otg",
	.resource		= comcerto_dwc_otg_resources,
	.num_resources	= ARRAY_SIZE(comcerto_dwc_otg_resources),
	.dev = {
		.platform_data = NULL,
        .dma_mask               = &comcerto_dwc_otg_dmamask,
        .coherent_dma_mask      = DMA_BIT_MASK(32),
	}
};
#endif

#if defined(CONFIG_COMCERTO_SATA)
static struct ahci_platform_data comcerto_ahci_pdata = {
        .init = comcerto_ahci_init,
};

static struct resource comcerto_ahci_resource[] = {
        [0] = {
                .start  = COMCERTO_AXI_SATA_BASE,
                .end    = COMCERTO_AXI_SATA_BASE + SZ_64K - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = IRQ_SATA,
                .end    = IRQ_SATA,
                .flags  = IORESOURCE_IRQ,
        },
};

static u64 comcerto_ahci_dmamask = DMA_BIT_MASK(32);

struct platform_device comcerto_device_ahci = {
        .name           = "ahci",
        .id             = -1,
        .resource       = comcerto_ahci_resource,
        .num_resources  = ARRAY_SIZE(comcerto_ahci_resource),
        .dev            = {
                .platform_data          = &comcerto_ahci_pdata,
                .dma_mask               = &comcerto_ahci_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
        },
};
#endif

/* --------------------------------------------------------------------
 *  XOR Engine
 * -------------------------------------------------------------------- */

static struct resource comcerto_xor_resource[] = {
		{
				.name  = "xor base address",
				.start = COMCERTO_APB_MDMA_BASE,
				.end   = COMCERTO_APB_MDMA_BASE + COMCERTO_APB_MDMA_SIZE - 1,
				.flags = IORESOURCE_MEM,
		},
		{
				.name  = "IO2M IRQ",
				.start = IRQ_MDMA_IO2M,
				.flags = IORESOURCE_IRQ,
		},
};

static u64 comcerto_xor_dma_mask = DMA_BIT_MASK(32);

static struct platform_device comcerto_xor_device = {
	.name       = "comcerto_xor",
	.id         = 0,
	.dev        = {
		.dma_mask    = &comcerto_xor_dma_mask,
		.coherent_dma_mask    = DMA_BIT_MASK(32),
	},
	.num_resources  = ARRAY_SIZE(comcerto_xor_resource),
	.resource = comcerto_xor_resource,
};

/* --------------------------------------------------------------------
 *  Basic C2K MDMA Engine
 * -------------------------------------------------------------------- */

static struct resource comcerto_dma_resource[] = {
		{
				.name  = "c2k mdma base address",
				.start = COMCERTO_APB_MDMA_BASE,
				.end   = COMCERTO_APB_MDMA_BASE + COMCERTO_APB_MDMA_SIZE - 1,
				.flags = IORESOURCE_MEM,
		},
		{
				.name  = "IO2M IRQ",
				.start = IRQ_MDMA_IO2M,
				.flags = IORESOURCE_IRQ,
		},
};

static u64 comcerto_dma_dma_mask = DMA_BIT_MASK(32);

static struct platform_device comcerto_dma_device = {
	.name       = "comcerto_dma",
	.id         = 0,
	.dev        = {
		.dma_mask    = &comcerto_dma_dma_mask,
		.coherent_dma_mask    = DMA_BIT_MASK(32),
	},
	.num_resources  = ARRAY_SIZE(comcerto_dma_resource),
	.resource = comcerto_dma_resource,
};

#if defined(CONFIG_COMCERTO_EPAVIS)
static u64 comcerto_epavis_cie_dmamask = DMA_BIT_MASK(32);

struct platform_device comcerto_device_epavis_cie = {
        .name           = "lc_cie",
        .id             = -1,
        .dev            = {
                .platform_data          = NULL,
                .dma_mask               = &comcerto_epavis_cie_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
        },
};
static u64 comcerto_epavis_decomp_dmamask = DMA_BIT_MASK(32);

struct platform_device comcerto_device_epavis_decomp = {
        .name           = "lc_decomp",
        .id             = -1,
        .dev            = {
                .platform_data          = NULL,
                .dma_mask               = &comcerto_epavis_decomp_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
        },
};
#endif

#if defined(CONFIG_COMCERTO_CSYS_TPI_CLOCK) 
struct platform_device comcerto_device_tpi_csys_clk = {
	.name		= "tpi_csys",
	.id		= -1,
	.dev		=  {
		.platform_data		= NULL,
	},
};
#endif

int usb3_clk_internal = 1;
static int __init get_usb3_clk_mode(char *str)
{
        if (!strcmp(str, "no"))
                usb3_clk_internal = 0;

        return 1;
}

__setup("usb3_internal_clk=", get_usb3_clk_mode);

int is_mac_zero(u8 *buf)
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
		*mac_addr_str++;
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

int comcerto_mac_addr_get(uint8_t mac[6], uint8_t index)
{
	if(index>=3)
		return -EINVAL;
	memcpy(mac, c2k_mac_addr[index], 6);
	return 0;
}
EXPORT_SYMBOL(comcerto_mac_addr_get);

static void __init mac_addr_setup(char *str)
{
	int str_incr_cnt = 0;

	if (*str++ != '=' || !*str)  /* No mac addr specified */
		return;

	str_incr_cnt = mac_addr_atoi(c2k_mac_addr[0], str);

	str += str_incr_cnt;

	str_incr_cnt = mac_addr_atoi(c2k_mac_addr[1], str);

	str += str_incr_cnt;

	mac_addr_atoi(c2k_mac_addr[2], str);
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

static struct platform_device *comcerto_common_devices[] __initdata = {
#if defined(CONFIG_COMCERTO_UART0_SUPPORT) || defined(CONFIG_COMCERTO_UART1_SUPPORT)
	&comcerto_uart,
#endif
#ifdef CONFIG_COMCERTO_MSP
	&comcerto_ved,
#endif  /* CONFIG_COMCERTO_MSP */
	&comcerto_pmu,

#if defined(CONFIG_COMCERTO_USB3_SUPPORT)
	&comcerto_device_usb3,
#endif

#if defined(CONFIG_COMCERTO_USB2_SUPPORT)
	&comcerto_dwc_otg_device,
#endif	

#if defined(CONFIG_COMCERTO_SATA)
	&comcerto_device_ahci,
#endif
//	&comcerto_xor_device,
	&comcerto_dma_device,
#if defined(CONFIG_COMCERTO_EPAVIS)
	&comcerto_device_epavis_cie,
	&comcerto_device_epavis_decomp,
#endif
#if defined(CONFIG_COMCERTO_CSYS_TPI_CLOCK)
	&comcerto_device_tpi_csys_clk,
#endif
};

void __init device_init(void)
{
	/* Default value for the bit mask */
	unsigned int default_host_utilpe_shared_bitmask = ~(USB2p0_IRQ|WOL_IRQ);
	struct clk *axi_clk,*ddr_clk,*arm_clk,*l2cc_clk;
	HAL_clk_div_backup_relocate_table ();
	system_rev = (readl(COMCERTO_GPIO_DEVICE_ID_REG) >> 24) & 0xf;

	/* Initialize the reset driver here */
	reset_init();
	
	/* Enable the AXI,DDR,A9 susbsystem clock
	 * this is just for s/w use count house keeping.
	 * These clocks will never be disabled from bootloader while
	 * booting. In fact we are keeping this because for these clocks , 
	 * we dont have any driver to do a clk_enable. so doing  it here.
	*/

	/* Get the AXI clk structure */
	axi_clk = clk_get(NULL,"axi");
	if (IS_ERR(axi_clk)) {
		pr_err("%s: Unable to obtain AXI clock: %ld\n",__func__,PTR_ERR(axi_clk));
		/* System cannot proceed from here */
		BUG();
	}
	/* Enable the AXI clk  */
	if (clk_enable(axi_clk)){
		pr_err("%s: Unable to enable AXI clock:\n",__func__);
		/* System cannot proceed from here */
		BUG();
	}
	
	/* Get the DDR clock structure */
	ddr_clk = clk_get(NULL,"ddr");
	if (IS_ERR(ddr_clk)) {
		pr_err("%s: Unable to obtain DDR clock: %ld\n",__func__,PTR_ERR(ddr_clk));
		/* System cannot proceed from here */
		BUG();
	}
	/* Enable the DDR clk  */
 	if (clk_enable(ddr_clk)){
		pr_err("%s: Unable to enable DDR clock:\n",__func__);
		/* System cannot proceed from here */
		BUG();
	}
	
	/* Get the CPU(A9) clock */
	arm_clk = clk_get(NULL,"arm");
	if (IS_ERR(arm_clk)) {
		pr_err("%s: Unable to obtain A9(arm) clock: %ld\n",__func__,PTR_ERR(arm_clk));
		/* System cannot proceed from here */
		BUG();
	}
	/* Enable the ARM clk  */
	if (clk_enable(arm_clk)){
		pr_err("%s: Unable to enable A9(arm) clock:\n",__func__);
		/* System cannot proceed from here */
		BUG();
	}

	/* Get the L2CC clock */
	l2cc_clk = clk_get(NULL,"l2cc");
	if (IS_ERR(l2cc_clk)) {
		pr_err("%s: Unable to obtain L2CC clock: %ld\n",__func__,PTR_ERR(l2cc_clk));
		/* L2CC initilization cannot proceed from here */
		BUG();
	}

	/* Enable the L2CC clk  */
	if (clk_enable(l2cc_clk)){
		pr_err("%s: Unable to enable L2CC clock:\n",__func__);
		/* L2CC initilization cannot proceed from here */
		BUG();
	}
	
#ifdef CONFIG_CACHE_L2X0
	comcerto_l2cc_init();
#endif

	exp_bus_init();

	gpio_init();

#ifdef CONFIG_COMCERTO_TDM_CLOCK
	// [FIXME] Take TDM out of reset
	//writel(readl(COMCERTO_BLOCK_RESET_REG) | TDM_RST, COMCERTO_BLOCK_RESET_REG);
#endif
	/* Default bit mask is applied here , which will be passed to Util-Pe*/
	c2k_pm_bitmask_store(default_host_utilpe_shared_bitmask);

	platform_add_devices(comcerto_common_devices, ARRAY_SIZE(comcerto_common_devices));
}

void __init platform_reserve(void)
{
	/* boot_secondary() in arch/arm/mach-comcerto/platsmp.c uses this range
	 * to store a jump instruction. The second CPU executes this
	 * instruction when it comes out of reset.*/
	if (memblock_reserve(0, 0x24) < 0)
		BUG();

	if (memblock_reserve((phys_addr_t) get_antirebootloop_ptr(),
				PAGE_SIZE) < 0)
		BUG();

	/* Allocate DDR block used by PFE/MSP, the base address is fixed so that util-pe code can
	be linked at a fixed address */
	if (memblock_reserve(COMCERTO_DDR_SHARED_BASE, COMCERTO_DDR_SHARED_SIZE) < 0)
		BUG();

	if (memblock_free(COMCERTO_DDR_SHARED_BASE, COMCERTO_DDR_SHARED_SIZE) < 0)
		BUG();

	if (memblock_remove(COMCERTO_DDR_SHARED_BASE, COMCERTO_DDR_SHARED_SIZE) < 0)
		BUG();
}

phys_addr_t get_antirebootloop_ptr(void) {
	return COMCERTO_AXI_DDR_BASE + (1 * PAGE_SIZE);
}
