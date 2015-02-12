/*
 * arch/arm/mach-comcerto/board-optimus.c
 *
 *  Copyright (C) 2012 Mindspeed Technologies, Inc.
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

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/serial_8250.h>
#include <linux/memblock.h>
#include <linux/phy.h>

#include <linux/mtd/mtd.h>
#if defined(CONFIG_MTD_NAND_COMCERTO) || defined(CONFIG_MTD_NAND_COMCERTO_MODULE)
#include <linux/mtd/nand.h>
#endif
#include <linux/mtd/partitions.h>

#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI_MSPD_HIGH_SPEED)
#include <linux/spi/spi.h>
#endif

#if defined(CONFIG_COMCERTO_I2C_SUPPORT)
#include <linux/i2c.h>
#endif

#include <asm/sizes.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/io.h>

#include <asm/mach/flash.h>
#include <asm/mach/arch.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/dma.h>
#include <linux/dw_dmac.h>

#include <linux/clockchips.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <asm/smp_twd.h>
#include <asm/localtimer.h>
#include <asm/hardware/gic.h>
#include <asm/mach/time.h>
#include <mach/gpio.h>


extern void platform_reserve(void);
extern void device_map_io (void);
extern void device_irq_init(void);
extern void device_init(void);
extern void mac_addr_init(struct comcerto_pfe_platform_data *);
extern struct sys_timer comcerto_timer;

static void __init board_gpio_init(void)
{
#ifdef CONFIG_COMCERTO_PFE_UART_SUPPORT
	writel((readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~PFE_UART_GPIO) | PFE_UART_BUS, COMCERTO_GPIO_PIN_SELECT_REG);
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= PFE_UART_GPIO_PIN; /* GPIOs 12 & 13 are used for PFE_UART */
#endif

#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI2_MSPD_LOW_SPEED)
	/* enable SPI pins */
	writel((readl(COMCERTO_GPIO_PIN_SELECT_REG1) & ~(SPI_MUX_GPIO_1)) | (SPI_MUX_BUS_1), COMCERTO_GPIO_PIN_SELECT_REG1);
	writel((readl(COMCERTO_GPIO_63_32_PIN_SELECT) & ~(SPI_MUX_GPIO_2)) | (SPI_MUX_BUS_2), COMCERTO_GPIO_63_32_PIN_SELECT);
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= SPI_MUX_GPIO_1_PIN; /* GPIOs 18,19, 21,22, 30,31 are used for SPI*/
	c2k_gpio_pin_stat.c2k_gpio_pins_32_63 |= SPI_MUX_GPIO_2_PIN; /* GPIO 32 is used for SPI*/
#endif

#if defined(CONFIG_SPI_MSPD_HIGH_SPEED)
	/* enable SPI pins */
	writel((readl(COMCERTO_GPIO_PIN_SELECT_REG1) & ~(SPI_2_MUX_GPIO_1)) | (SPI_2_MUX_BUS_1), COMCERTO_GPIO_PIN_SELECT_REG1);
	writel((readl(COMCERTO_GPIO_63_32_PIN_SELECT) & ~(SPI_2_MUX_GPIO_2)) | (SPI_2_MUX_BUS_2), COMCERTO_GPIO_63_32_PIN_SELECT);
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= SPI_2_MUX_GPIO_1_PIN;
	c2k_gpio_pin_stat.c2k_gpio_pins_32_63 |= SPI_2_MUX_GPIO_2_PIN;
#endif

#if defined(CONFIG_COMCERTO_I2C_SUPPORT)
	writel((readl(COMCERTO_GPIO_PIN_SELECT_REG1) & ~I2C_GPIO) | I2C_BUS, COMCERTO_GPIO_PIN_SELECT_REG1);
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= I2C_GPIO_PIN;
#endif

#if defined(CONFIG_MTD_NAND_COMCERTO) || defined(CONFIG_MTD_NAND_COMCERTO_MODULE)
	writel((readl(COMCERTO_GPIO_PIN_SELECT_REG1) & ~NAND_GPIO) | NAND_BUS, COMCERTO_GPIO_PIN_SELECT_REG1);
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= NAND_GPIO_PIN;
#endif

#if defined(CONFIG_MTD_COMCERTO_NOR)
	writel((readl(COMCERTO_GPIO_PIN_SELECT_REG1) & ~NOR_GPIO) | NOR_BUS, COMCERTO_GPIO_PIN_SELECT_REG1);
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= NOR_GPIO_PIN;
#endif

#ifdef MOCA_RESET_GPIO_PIN
	__raw_writel(__raw_readl(COMCERTO_GPIO_OUTPUT_REG) | MOCA_RESET_GPIO_PIN, COMCERTO_GPIO_OUTPUT_REG);
#endif

#ifdef PCIE_ADDITIONAL_RESET_PIN
	printk(KERN_WARNING "Pulsing GPIO_62 to reset PCIe");
	writel((0x2 << 4) | (readl(COMCERTO_GPIO_MISC_PIN_SELECT) & ~(0x3 << 4)),
			COMCERTO_GPIO_MISC_PIN_SELECT);
	writel(readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) & ~(PCIE_ADDITIONAL_RESET_PIN),
			COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
	c2k_gpio_pin_stat.c2k_gpio_pins_32_63 |= PCIE_ADDITIONAL_RESET_PIN;
	writel(readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~(PCIE_ADDITIONAL_RESET_PIN),
			COMCERTO_GPIO_63_32_PIN_OUTPUT);
	udelay(1000);
	writel(readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) | (PCIE_ADDITIONAL_RESET_PIN),
			COMCERTO_GPIO_63_32_PIN_OUTPUT);
	udelay(1000);
#endif

	// enable GPIO0 interrupt (for MoCA) as level triggered, active high.
	__raw_writel(__raw_readl(COMCERTO_GPIO_INT_CFG_REG) | (0x3),
			COMCERTO_GPIO_INT_CFG_REG);
}

/* --------------------------------------------------------------------
 *  NOR device
 * -------------------------------------------------------------------- */
#if defined(CONFIG_MTD_COMCERTO_NOR)

static struct resource comcerto_nor_resources[] = {
	{
		.start	= NORFLASH_MEMORY_PHY1,
		.end	= NORFLASH_MEMORY_PHY1 + SZ_64M - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct flash_platform_data comcerto_nor_data = {
	.map_name	= "cfi_probe",
	.width	= 2,
};

static struct platform_device comcerto_nor = {
	.name           = "comcertoflash",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(comcerto_nor_resources),
	.resource       = comcerto_nor_resources,
	.dev = {
		.platform_data	= &comcerto_nor_data,
	},
};
#endif

static struct resource rtc_res[] = {
       {
               .start = COMCERTO_APB_RTC_BASE,
               .end = COMCERTO_APB_RTC_BASE + SZ_32 - 1,
               .flags = IORESOURCE_MEM,
       },
       {
               .start = IRQ_RTC_ALM,
               .flags = IORESOURCE_IRQ,
       },
       {
               .start = IRQ_RTC_PRI,
               .flags = IORESOURCE_IRQ,
       },
};
static struct platform_device rtc_dev = {
       .name = "c2k-rtc",
       .id = -1,
       .num_resources = ARRAY_SIZE(rtc_res),
       .resource = rtc_res,
};

/* --------------------------------------------------------------------
 *  DMAC controller
 * -------------------------------------------------------------------- */
#if defined(CONFIG_COMCERTO_DW_DMA_SUPPORT)
static struct resource dw_dmac_resource[] = {
	{
		.start          = DW_DMA_DMAC_BASEADDR,
		.end            = DW_DMA_DMAC_BASEADDR + 0x2C0,
		.flags          = IORESOURCE_MEM,
	},
	{
		.start          = IRQ_DMAC,
		.flags          = IORESOURCE_IRQ,
	}
};

static struct dw_dma_platform_data dw_dmac_data = {
	.nr_channels    = 8,
};

static u64 dw_dmac_dma_mask = DMA_BIT_MASK(32);

static struct platform_device dw_dmac_device = {
	.name           = "dw_dmac",
	.id             = 0,
	.dev            = {
		.dma_mask = &dw_dmac_dma_mask,
		.platform_data  = &dw_dmac_data,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.resource       = dw_dmac_resource,
	.num_resources  = ARRAY_SIZE(dw_dmac_resource),
};
#endif

/* --------------------------------------------------------------------
 *  NAND device
 * -------------------------------------------------------------------- */
#if defined(CONFIG_MTD_NAND_COMCERTO) || defined(CONFIG_MTD_NAND_COMCERTO_MODULE)
static struct resource comcerto_nand_resources[] = {
	{
		.start	= COMCERTO_NAND_FIO_ADDR,
		.end	= COMCERTO_NAND_FIO_ADDR + COMCERTO_NAND_IO_SZ - 1,
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device comcerto_nand = {
	.name		= "comcertonand",
	.id		= -1,
	.dev		= {
				.platform_data	= NULL,
	},
	.resource	= comcerto_nand_resources,
	.num_resources	= ARRAY_SIZE(comcerto_nand_resources),
};
#endif

/* --------------------------------------------------------------------
 *  SPI bus controller
 * -------------------------------------------------------------------- */
#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI_MSPD_HIGH_SPEED)

#define	CLK_NAME	10
struct spi_controller_pdata {
	int use_dma;
	int num_chipselects;
	int bus_num;
	u32 max_freq;
	char clk_name[CLK_NAME];
};

struct spi_platform_data {
	int type;
	int dummy;
};

struct spi_controller_data {
        u8 poll_mode;   /* 0 for contoller polling mode */
        u8 type;        /* SPI/SSP/Micrwire */
        u8 enable_dma;
        void (*cs_control)(u32 command);
};

struct spi_platform_data spi_pdata = {
	.type = 0,
	.dummy = 0,
};

struct spi_controller_data spi_ctrl_data =  {
        .poll_mode = 1,
};

// We list more than one bmoca in here because it was attached to different
// chip selects in different variations of the board. We have to trust the
// driver to reject instances that aren't actually present.
static struct spi_board_info comcerto_spi_board_info[] = {
	{
		.modalias = "bmoca",
		.chip_select = 0,
		.max_speed_hz = 20*1000*1000,
		.bus_num = 0,
		.irq = IRQ_G0,
		.mode = SPI_MODE_3,
		.platform_data = &spi_pdata,
                .controller_data = &spi_ctrl_data,
	},

	{
		.modalias = "bmoca",
		.chip_select = 0,
		.max_speed_hz = 20*1000*1000,
		.bus_num = 1,
		.irq = IRQ_G0,
		.mode = SPI_MODE_3,
		.platform_data = &spi_pdata,
                .controller_data = &spi_ctrl_data,
	},
};
#endif

#if defined(CONFIG_SPI_MSPD_HIGH_SPEED)
struct spi_controller_pdata fast_spi_pdata = {
	.use_dma = 0,
	.num_chipselects = 2,
	.bus_num = 1,
	.max_freq = 50 * 1000 * 1000,
	.clk_name = "DUS",
};
#endif

#if defined(CONFIG_SPI_MSPD_HIGH_SPEED) || defined(CONFIG_SPI2_MSPD_HIGH_SPEED)
static struct resource comcerto_fast_spi_resource[] = {
	{
		.start  = COMCERTO_AXI_SPI_BASE,
		.end    = COMCERTO_AXI_SPI_BASE + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = IRQ_SPI,
		.flags  = IORESOURCE_IRQ,
	}
};

static struct platform_device comcerto_fast_spi = {
	.name = "comcerto_spi",
	.id = 1,
	.num_resources = ARRAY_SIZE(comcerto_fast_spi_resource),
	.resource = comcerto_fast_spi_resource,
#if defined(CONFIG_SPI_MSPD_HIGH_SPEED)
	.dev = {
		.platform_data = &fast_spi_pdata,
	},
#endif
};
#endif

#if defined(CONFIG_SPI_MSPD_LOW_SPEED)
struct spi_controller_pdata ls_spi_pdata = {
	.use_dma = 0,
	.num_chipselects = 4,
	.bus_num = 0,
	.max_freq = 20 * 1000 * 1000,
	.clk_name = "spi_i2c",
};
#endif

#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI2_MSPD_LOW_SPEED)
static struct resource comcerto_spi_resource[] = {
	{
		.start  = COMCERTO_APB_SPI_BASE,
		.end    = COMCERTO_APB_SPI_BASE + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = IRQ_SPI_LS,
		.flags  = IORESOURCE_IRQ,
	}
};

static struct platform_device comcerto_spi = {
	.name = "comcerto_spi",
	.id = 0,
	.num_resources = ARRAY_SIZE(comcerto_spi_resource),
	.resource = comcerto_spi_resource,
#if defined(CONFIG_SPI_MSPD_LOW_SPEED)
	.dev = {
		.platform_data = &ls_spi_pdata,
	},
#endif
};
#endif

/* --------------------------------------------------------------------
 *  I2C bus controller
 * -------------------------------------------------------------------- */
#if defined(CONFIG_COMCERTO_I2C_SUPPORT)
static struct resource comcerto_i2c_resources[] = {
	{
		.start	= COMCERTO_APB_I2C_BASE,
		.end	= COMCERTO_APB_I2C_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_I2C,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct i2c_board_info comcerto_i2c_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("lm96163", 0x4c),
	},
	{
		I2C_BOARD_INFO("24c512", 0x50),
	},
#if defined(CONFIG_GOOGLE_SPACECAST)
	{
		I2C_BOARD_INFO("tpm_i2c_infineon", 0x20),
	},
	{
		I2C_BOARD_INFO("tps65233", 0x60),
	},
	{
		I2C_BOARD_INFO("m88rs6000", 0x69),
	},
#endif
};

static struct platform_device comcerto_i2c = {
	.name           = "comcerto_i2c",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(comcerto_i2c_resources),
	.resource       = comcerto_i2c_resources,
};
#endif

/* --------------------------------------------------------------------
*  Watchdog
* -------------------------------------------------------------------- */
#ifdef CONFIG_MPCORE_WATCHDOG
static struct resource comcerto_a9wd_resources[] = {
	{
		.start	= COMCERTO_TWD_BASE,
		.end	= COMCERTO_TWD_BASE + 0xFF,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "mpcore_wdt",
		.start	= IRQ_LOCALWDOG,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device comcerto_a9wd = {
	.name		= "mpcore_wdt",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(comcerto_a9wd_resources),
	.resource       = comcerto_a9wd_resources,
};
#endif

#ifdef CONFIG_COMCERTO_WATCHDOG
static struct resource comcerto_wdt_resources[] = {
	{
		.start	= COMCERTO_APB_TIMER_BASE + 0xD0,
		.end	= COMCERTO_APB_TIMER_BASE + 0xD8,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device comcerto_wdt = {
        .name   = "comcerto_wdt",
        .id     = -1,
	.num_resources  = ARRAY_SIZE(comcerto_wdt_resources),
	.resource       = comcerto_wdt_resources,
};
#endif

#if defined(CONFIG_COMCERTO_ELP_SUPPORT)
/* --------------------------------------------------------------------
 *  IPsec
 * -------------------------------------------------------------------- */
static struct resource comcerto_elp_resources[] = {
	{
		.name   = "elp",
		.start  = COMCERTO_AXI_SPACC_PDU_BASE,
		.end    = COMCERTO_AXI_SPACC_PDU_BASE + SZ_16M  - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name   = "irq_spacc",
		.start  = IRQ_SPACC,
		.end    = IRQ_SPACC,
		.flags  = IORESOURCE_IRQ,
	}
};

static u64 comcerto_elp_dma_mask = DMA_BIT_MASK(32);

static struct platform_device  comcerto_elp_device = {
	.name                   = "Elliptic-EPN1802",
	.id                     = 0,
	.num_resources          = 2,
	.resource               = comcerto_elp_resources,
	.dev = {
		.dma_mask               = &comcerto_elp_dma_mask,
		.coherent_dma_mask      = DMA_BIT_MASK(32),
	},
};
#endif

static struct comcerto_tdm_data comcerto_tdm_pdata = {
	.fsoutput = 1, /* Generic Pad Control and Version ID Register[2] */
	.fspolarity = 0, /* 28 FSYNC_FALL(RISE)_EDGE */
	.fshwidth = 1, /* High_Phase_Width[10:0] */
	.fslwidth = 0xFF, /* Low_Phase_Width[10:0] */
	.clockhz = 2048000, /* INC_VALUE[29:0] According to the desired TDM clock output frequency, this field should be configured */
	.clockout = 1, /* 0 -> set bit 21, clear bit 20 in COMCERTO_GPIO_IOCTRL_REG
			  (software control, clock input)
			  1 -> set bit 21 and 20 in COMCERTO_GPIO_IOCTRL_REG
			  (software control, clock output)
			  2 -> clear bit 21 in COMCERTO_GPIO_IOCTRL_REG (hardware control) */
	.tdmmux = 0x2, /* TDM interface Muxing:0x0 - TDM block, 0x1 - ZDS block,
		0x2 - GPIO[63:60] signals and 0x3 - MSIF block is selected */
#if 0
	/* FIX ME - Need correct values for TDM_DR, TDM_DX, TDM_FS and TDM_CK */
	.tdmck = 0x3F,
	.tdmfs = 0x3F,
	.tdmdx = 0x3F,
	.tdmdr = 0x3F,
#endif
};

static struct platform_device comcerto_tdm_device = {
	.name	= "comcerto-tdm",
	.id		= 0,
	.dev.platform_data = &comcerto_tdm_pdata,
	.num_resources	= 0,
	.resource = NULL,
};

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

	{
		.name	= "hif",
		.start  = IRQ_PFE_HIF,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct comcerto_pfe_platform_data comcerto_pfe_pdata = {
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

static u64 comcerto_pfe_dma_mask = DMA_BIT_MASK(32);

static struct platform_device comcerto_pfe_device = {
	.name		= "pfe",
	.id		= 0,
	.dev		= {
		.platform_data		= &comcerto_pfe_pdata,
		.dma_mask		= &comcerto_pfe_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(comcerto_pfe_resources),
	.resource	= comcerto_pfe_resources,
};

static struct platform_device *comcerto_devices[] __initdata = {
#if defined(CONFIG_MTD_NAND_COMCERTO) || defined(CONFIG_MTD_NAND_COMCERTO_MODULE)
		&comcerto_nand,
#endif
#if defined(CONFIG_MTD_COMCERTO_NOR)
		&comcerto_nor,
#endif
#if defined(CONFIG_COMCERTO_I2C_SUPPORT)
		&comcerto_i2c,
#endif

#if defined (CONFIG_MPCORE_WATCHDOG)
		&comcerto_a9wd,
#endif

#if defined(CONFIG_COMCERTO_WATCHDOG)
		&comcerto_wdt,
#endif

#if defined(CONFIG_SPI_MSPD_HIGH_SPEED) || defined(CONFIG_SPI2_MSPD_HIGH_SPEED)
		&comcerto_fast_spi,
#endif
#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI2_MSPD_LOW_SPEED)
		&comcerto_spi,
#endif
#if defined(CONFIG_COMCERTO_DW_DMA_SUPPORT)
		&dw_dmac_device,
#endif
		&comcerto_tdm_device,
		&comcerto_pfe_device,
		&rtc_dev,
#if defined(CONFIG_COMCERTO_ELP_SUPPORT)
	&comcerto_elp_device,
#endif
};


/************************************************************************
 *  Expansion bus
 *
 ************************************************************************/
/* This variable is used by comcerto-2000.c to initialize the expansion bus */
int comcerto_exp_values[5][7]= {
	/* ENABLE, BASE, SEG_SZ, CFG, TMG1, TMG2, TMG3 */
	{1, (EXP_BUS_REG_BASE_CS0 >> 12), ((EXP_BUS_REG_BASE_CS0 + EXP_CS0_SEG_SIZE - 1) >> 12), EXP_MEM_BUS_SIZE_16, 0x1A1A401F, 0x06060A04, 0x00000002},		/*TODO Values to check*/
	{0, (EXP_BUS_REG_BASE_CS1 >> 12), ((EXP_BUS_REG_BASE_CS1 + EXP_CS1_SEG_SIZE - 1) >> 12), EXP_RDY_EN|EXP_MEM_BUS_SIZE_32, 0x1A1A401F, 0x06060A04, 0x00000002},	/*TODO Values to check*/
	{0, (EXP_BUS_REG_BASE_CS2 >> 12), ((EXP_BUS_REG_BASE_CS2 + EXP_CS2_SEG_SIZE - 1) >> 12), EXP_STRB_MODE|EXP_ALE_MODE|EXP_MEM_BUS_SIZE_8, 0x1A10201A, 0x03080403, 0x0000002},	/*TODO Values to check*/
	{0, (EXP_BUS_REG_BASE_CS3 >> 12), ((EXP_BUS_REG_BASE_CS3 + EXP_CS3_SEG_SIZE - 1) >> 12), EXP_STRB_MODE|EXP_ALE_MODE|EXP_MEM_BUS_SIZE_8, 0x1A10201A, 0x03080403, 0x0000002},	/*BT8370*/
	{0, (EXP_BUS_REG_BASE_CS4 >> 12), ((EXP_BUS_REG_BASE_CS4 + EXP_CS4_SEG_SIZE - 1) >> 12), EXP_NAND_MODE|EXP_MEM_BUS_SIZE_8, 0x01010001, 0x01010101, 0x00000001},	/* NAND: TODO Values to check */
};

/************************************************************************
 *  Machine definition
 *
 ************************************************************************/
static void __init platform_map_io(void)
{
	device_map_io();
}

static void __init platform_irq_init(void)
{
	device_irq_init();
}

static void __init platform_init(void)
{
	device_init();
	board_gpio_init();

#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI_MSPD_HIGH_SPEED)
	spi_register_board_info(comcerto_spi_board_info, ARRAY_SIZE(comcerto_spi_board_info));
#endif
#if defined(CONFIG_COMCERTO_I2C_SUPPORT)
	i2c_register_board_info(0, comcerto_i2c_board_info, ARRAY_SIZE(comcerto_i2c_board_info));
#endif
	mac_addr_init(&comcerto_pfe_pdata);

	platform_add_devices(comcerto_devices, ARRAY_SIZE(comcerto_devices));
}

MACHINE_START(COMCERTO, "Google Fiber Optimus")
	.atag_offset    = COMCERTO_AXI_DDR_BASE + 0x100,
	.reserve	= platform_reserve,
	.map_io		= platform_map_io,
	.init_irq	= platform_irq_init,
	.init_machine	= platform_init,
	.timer		= &comcerto_timer,
#ifdef CONFIG_ZONE_DMA
	.dma_zone_size	= SZ_32M + 3*SZ_4M,
#endif
MACHINE_END
