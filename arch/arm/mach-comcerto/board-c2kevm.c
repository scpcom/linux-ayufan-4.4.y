#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
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

#include <linux/synobios.h>

#ifdef  MY_ABC_HERE
extern char gszSynoHWVersion[];
#endif
#include <asm/sizes.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/io.h>

#ifdef MY_ABC_HERE
#include <linux/spi/flash.h>
#else
#include <asm/mach/flash.h>
#endif
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
#if defined(MY_ABC_HERE)
#include <linux/synobios.h>
#endif

#ifdef MY_ABC_HERE
#include <linux/serial_reg.h>
#endif

#ifdef MY_ABC_HERE
#include <linux/module.h>
#endif

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
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= PFE_UART_GPIO_PIN;  
#endif

#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI2_MSPD_LOW_SPEED)
	 
	writel((readl(COMCERTO_GPIO_PIN_SELECT_REG1) & ~(SPI_MUX_GPIO_1)) | (SPI_MUX_BUS_1), COMCERTO_GPIO_PIN_SELECT_REG1);
	writel((readl(COMCERTO_GPIO_63_32_PIN_SELECT) & ~(SPI_MUX_GPIO_2)) | (SPI_MUX_BUS_2), COMCERTO_GPIO_63_32_PIN_SELECT);
	c2k_gpio_pin_stat.c2k_gpio_pins_0_31 |= SPI_MUX_GPIO_1_PIN;  
	c2k_gpio_pin_stat.c2k_gpio_pins_32_63 |= SPI_MUX_GPIO_2_PIN;  
#endif

#if defined(CONFIG_SPI_MSPD_HIGH_SPEED)
	 
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
}

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

#if defined(CONFIG_COMCERTO_DW_DMA_SUPPORT)
static struct resource dw_dmac_resource[] = {
	{
		.start          = DW_DMA_DMAC_BASEADDR,
		.end            = DW_DMA_DMAC_BASEADDR + 0x400,
		.flags          = IORESOURCE_MEM,
	},
	{
		.start          = IRQ_DMAC,
		.flags          = IORESOURCE_IRQ,
	}
};

static struct dw_dma_platform_data dw_dmac_data = {
	.nr_channels    = 8,
	.chan_priority = 1,
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

#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI_MSPD_HIGH_SPEED)

struct spi_flash_platform_data {
       char            *name;
       struct mtd_partition *parts;
       unsigned int    nr_parts;
       char            *type;
       u32             num_resources;
       struct resource * resource;
        
};

#define	CLK_NAME	10
struct spi_controller_pdata {
	int use_dma;
	int num_chipselects;
	int bus_num;
	u32 max_freq;
	char clk_name[CLK_NAME];
	char type[32];
};

struct spi_platform_data {
	int type;
	int dummy;
};

struct spi_controller_data {
        u8 poll_mode;    
        u8 type;         
        u8 enable_dma;
        void (*cs_control)(u32 command);
};

struct spi_controller_data spi_ctrl_data =  {
        .poll_mode = 1,
};

struct spi_platform_data spi_pdata = {
	.type = 0,
	.dummy = 0,
};

struct spi_platform_data fast_spi_pdata = {
	.type = 0,
	.dummy = 0,
};

#if defined(CONFIG_SPI_MSPD_HIGH_SPEED)
struct spi_controller_pdata hs_spi_pdata = {
	.use_dma = 1,
	.num_chipselects = 2,
	.bus_num = 1,
 
	.max_freq = 5 * 1000 * 1000,
	.clk_name = "DUS",
	.type="m25p80",
};
#endif

static struct resource m25p80_flash_resource[] = {
	{
		.start  = COMCERTO_FASTSPI_IRAM_LOC,
		.end    = COMCERTO_FASTSPI_IRAM_LOC + COMCERTO_FASTSPI_IRAM_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct spi_flash_platform_data comcerto_spi_flash_data = {
	.num_resources = ARRAY_SIZE(m25p80_flash_resource),
	.resource = m25p80_flash_resource,
};

#ifdef MY_ABC_HERE
struct mtd_partition syno_c2k_16m_spi[] = {
    {
        .name       = "RedBoot",
        .size       = 0x00070000,
        .offset     = 0,
    }, {
        .name       = "zImage",
        .size       = 0x00500000,
        .offset     = 0x00070000,
    }, {
        .name       = "rd.gz",
        .size       = 0x00A60000,
        .offset     = 0x00570000,
    }, {
        .name       = "vendor",
        .size       = 0x00010000,
        .offset     = 0x00FD0000,
    }, {
        .name       = "RedBoot Config",
        .size       = 0x00010000,
        .offset     = 0x00FE0000,
    }, {
        .name       = "FIS directory",
        .size       = 0x00010000,
        .offset     = 0x00FF0000,
    },
};

const struct flash_platform_data syno_c2k_16m_flash = {
    .name       = "spi_flash",
    .parts      = syno_c2k_16m_spi,
    .nr_parts   = ARRAY_SIZE(syno_c2k_16m_spi),
};

static struct spi_board_info synology_spi_16m_info[] = {
	{
		.modalias = "n25q128a13",
		.chip_select = 0,
		.max_speed_hz = 30*1000*1000,
		.bus_num = 1,
		.irq = -1,
		.mode = SPI_MODE_3,
		.platform_data = &syno_c2k_16m_flash,
	},
};

struct mtd_partition syno_c2k_spi[] = {
    {
        .name       = "RedBoot",
        .size       = 0x00070000,
        .offset     = 0,
    }, {
        .name       = "zImage",
        .size       = 0x00300000,
        .offset     = 0x00070000,
    }, {
        .name       = "rd.gz",
        .size       = 0x00460000,
        .offset     = 0x00370000,
    }, {
        .name       = "vendor",
        .size       = 0x00010000,
        .offset     = 0x007D0000,
    }, {
        .name       = "RedBoot Config",
        .size       = 0x00010000,
        .offset     = 0x007E0000,
    }, {
        .name       = "FIS directory",
        .size       = 0x00010000,
        .offset     = 0x007F0000,
    },
};

const struct flash_platform_data syno_c2k_flash = {
    .name       = "spi_flash",
    .parts      = syno_c2k_spi,
    .nr_parts   = ARRAY_SIZE(syno_c2k_spi),
};

static struct spi_board_info synology_spi_info[] = {
	{
		.modalias = "n25q064",
		.chip_select = 0,
		.max_speed_hz = 30*1000*1000,
		.bus_num = 1,
		.irq = -1,
		.mode = SPI_MODE_3,
		.platform_data = &syno_c2k_flash,
	},
};
#endif

static struct spi_board_info comcerto_spi_board_info[] = {
	{
		 
		.modalias = "m25p80",
		.chip_select = 0,
		.max_speed_hz = 30*1000*1000,
		.bus_num = 1,
		.irq = -1,
		.mode = SPI_MODE_3,
		.platform_data = &comcerto_spi_flash_data,
                .controller_data = &spi_ctrl_data,
	},
	{
		 
		.modalias = "proslic",
		.max_speed_hz = 4*1000*1000,
		.chip_select = 1,
		.mode = SPI_MODE_3,
		.bus_num = 0,
		.irq = -1,
	 
		.platform_data = &spi_pdata,
                .controller_data = &spi_ctrl_data,
	},
	{
		.modalias = "comcerto_spi3",
		.chip_select = 2,
		.max_speed_hz = 4*1000*1000,
		.bus_num = 0,
		.irq = -1,
		.mode = SPI_MODE_3,
		.platform_data = &spi_pdata,
                .controller_data = &spi_ctrl_data,
	},

#if 0  

	{
		.modalias = "proslic",
		.max_speed_hz = 2*1000*1000,
		.chip_select = 3,
		.mode = SPI_MODE_1,
		.bus_num = 0,
		.irq = -1,
		.mode = SPI_MODE_3,
		.platform_data = &spi_pdata,
                .controller_data = &spi_ctrl_data,
	},
#else
	{
		.modalias = "legerity",
		.chip_select = 3,
		.max_speed_hz = 4*1000*1000,
		.bus_num = 0,
		.irq = -1,
		.mode = SPI_MODE_3,
		.platform_data = &spi_pdata,
                .controller_data = &spi_ctrl_data,
	},
#endif
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
		.platform_data = &hs_spi_pdata,
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

static struct platform_device comcerto_i2c = {
	.name           = "comcerto_i2c",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(comcerto_i2c_resources),
	.resource       = comcerto_i2c_resources,
};
#endif

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
	.fsoutput = 1,  
	.fspolarity = 0,  
	.fshwidth = 1,  
	.fslwidth = 0xFF,  
	.clockhz = 2048000,  
	.clockout = 1,  
	.tdmmux = 0x1,  
#if 0
	 
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

#if defined(CONFIG_DSPG_DECT_CSS)
#define CSS_ITCM_BASE		COMCERTO_AXI_DECT_BASE
#define CSS_ITCM_SIZE		(SZ_1M)

#define CSS_DTCM_BASE		(CSS_ITCM_BASE + CSS_ITCM_SIZE)
#define CSS_DTCM_SIZE		(SZ_1M)

static struct resource comcerto_css_resources[] = {
	{
		.name	= "itcm",
		.start	= CSS_ITCM_BASE,
		.end	= CSS_ITCM_BASE + CSS_ITCM_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "dtcm",
		.start	= CSS_DTCM_BASE,
		.end	= CSS_DTCM_BASE + CSS_DTCM_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device comcerto_css_device = {
	.name		= "css",
	.id		= 0,
	.dev		= {
		.platform_data = 0,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(comcerto_css_resources),
	.resource	= comcerto_css_resources,
};
#endif

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
		.name = GEM0_ITF_NAME,
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_PHY_RGMII_ADD_DELAY,
		.bus_id = 0,
#if defined(MY_ABC_HERE)
		.phy_id = 1,
#else
		.phy_id = 4,
#endif
		.gem_id = 0,
		.mac_addr = (u8[])GEM0_MAC,
	},

	.comcerto_eth_pdata[1] = {
		.name = GEM1_ITF_NAME,
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
#if defined(MY_ABC_HERE)
		.phy_flags = GEMAC_PHY_RGMII_ADD_DELAY,
		.bus_id = 0,
		.phy_id = 2,
#else
		.phy_flags = GEMAC_NO_PHY,
#endif
		.gem_id = 1,
		.mac_addr = (u8[])GEM1_MAC,
	},

	.comcerto_eth_pdata[2] = {
		.name = GEM2_ITF_NAME,
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_NO_PHY,
		.gem_id = 2,
		.mac_addr = (u8[])GEM2_MAC,
	},

	.comcerto_mdio_pdata[0] = {
		.enabled = 1,
#if defined(MY_ABC_HERE)
		.phy_mask = 0xFFFFFFE0,
#else
		.phy_mask = 0xFFFFFFEF,
#endif
		.mdc_div = 96,
		.irq = {
			[4] = PHY_POLL,
		},
	},
};

#if defined(MY_ABC_HERE)
static struct comcerto_pfe_platform_data comcerto_pfe_pdata_ds215air = {
	.comcerto_eth_pdata[0] = {
		.name = GEM0_ITF_NAME,
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_PHY_RGMII_ADD_DELAY,
		.bus_id = 0,
		.phy_id = 4,
		.gem_id = 0,
		.mac_addr = (u8[])GEM0_MAC,
	},

	.comcerto_eth_pdata[1] = {
		.name = GEM1_ITF_NAME,
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_NO_PHY,
		.gem_id = 1,
		.mac_addr = (u8[])GEM1_MAC,
	},

	.comcerto_eth_pdata[2] = {
		.name = GEM2_ITF_NAME,
		.device_flags = CONFIG_COMCERTO_GEMAC,
		.mii_config = CONFIG_COMCERTO_USE_RGMII,
		.gemac_mode = GEMAC_SW_CONF | GEMAC_SW_FULL_DUPLEX | GEMAC_SW_SPEED_1G,
		.phy_flags = GEMAC_NO_PHY,
		.gem_id = 2,
		.mac_addr = (u8[])GEM2_MAC,
	},

	.comcerto_mdio_pdata[0] = {
		.enabled = 1,
		.phy_mask = 0xFFFFFFEF,
		.mdc_div = 96,
		.irq = {
			[4] = PHY_POLL,
		},
	},
};
#endif
static u64 comcerto_pfe_dma_mask = DMA_BIT_MASK(32);

#if defined(MY_ABC_HERE)
#define SYNO_PFE_NAME "pfe"
#endif
static struct platform_device comcerto_pfe_device = {
#if defined(MY_ABC_HERE)
	.name		= SYNO_PFE_NAME,
#else
	.name		= "pfe",
#endif
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
#if defined(CONFIG_DSPG_DECT_CSS)
		&comcerto_css_device,
#endif
#if defined(CONFIG_COMCERTO_ELP_SUPPORT)
	&comcerto_elp_device,
#endif
};

int comcerto_exp_values[5][7]= {
	 
	{1, (EXP_BUS_REG_BASE_CS0 >> 12), ((EXP_BUS_REG_BASE_CS0 + EXP_CS0_SEG_SIZE - 1) >> 12), EXP_MEM_BUS_SIZE_16, 0x03034007, 0x04040502, 0x00000002},		 
	{0, (EXP_BUS_REG_BASE_CS1 >> 12), ((EXP_BUS_REG_BASE_CS1 + EXP_CS1_SEG_SIZE - 1) >> 12), EXP_RDY_EN|EXP_MEM_BUS_SIZE_32, 0x1A1A401F, 0x06060A04, 0x00000002},	 
	{0, (EXP_BUS_REG_BASE_CS2 >> 12), ((EXP_BUS_REG_BASE_CS2 + EXP_CS2_SEG_SIZE - 1) >> 12), EXP_STRB_MODE|EXP_ALE_MODE|EXP_MEM_BUS_SIZE_8, 0x1A10201A, 0x03080403, 0x0000002},	 
	{0, (EXP_BUS_REG_BASE_CS3 >> 12), ((EXP_BUS_REG_BASE_CS3 + EXP_CS3_SEG_SIZE - 1) >> 12), EXP_STRB_MODE|EXP_ALE_MODE|EXP_MEM_BUS_SIZE_8, 0x1A10201A, 0x03080403, 0x0000002},	 
	{1, (EXP_BUS_REG_BASE_CS4 >> 12), ((EXP_BUS_REG_BASE_CS4 + EXP_CS4_SEG_SIZE - 1) >> 12), EXP_NAND_MODE|EXP_MEM_BUS_SIZE_8, 0x00000001, 0x01010001, 0x00000002},	 
};

static void __init platform_map_io(void)
{
	device_map_io();
}

static void __init platform_irq_init(void)
{
	device_irq_init();
}

#ifdef MY_ABC_HERE
int (*gpfnSynoWOLSet)(void) = NULL;
EXPORT_SYMBOL(gpfnSynoWOLSet);
#endif

#ifdef MY_ABC_HERE
#define UART0_REG(x)		(COMCERTO_AXI_UART0_VADDR + ((UART_##x) << 2))
#define SET8N1				0x3
#define SOFTWARE_SHUTDOWN	0x31
#define SOFTWARE_REBOOT		0x43

static void synology_power_off(void)
{
#ifdef MY_ABC_HERE
	if(gpfnSynoWOLSet) {
		if(-1 == gpfnSynoWOLSet()) {
			printk("\nSet syno pfe wake on lan fail!\n");
		}
	}
#endif
	writel(SET8N1, UART0_REG(LCR));
	writel(SOFTWARE_SHUTDOWN, UART0_REG(TX));
}

static void synology_restart(char mode, const char *cmd)
{
	writel(SET8N1, UART0_REG(LCR));
	writel(SOFTWARE_REBOOT, UART0_REG(TX));
}
#endif  

static void __init platform_init(void)
{
#if defined(MY_ABC_HERE)
	int i = 0;
#endif
	device_init();
	board_gpio_init();

#if defined(CONFIG_SPI_MSPD_LOW_SPEED) || defined(CONFIG_SPI_MSPD_HIGH_SPEED)
#ifdef MY_ABC_HERE
	if(0 == strncmp(gszSynoHWVersion, HW_DS215airv10, strlen(HW_DS215airv10))) {
		spi_register_board_info(synology_spi_16m_info, ARRAY_SIZE(synology_spi_16m_info));
	} else {
		spi_register_board_info(synology_spi_info, ARRAY_SIZE(synology_spi_info));
	}
#else
	spi_register_board_info(comcerto_spi_board_info, ARRAY_SIZE(comcerto_spi_board_info));
#endif
#endif
#if defined(MY_ABC_HERE)
	if(0 == strncmp(gszSynoHWVersion, HW_DS215airv10, strlen(HW_DS215airv10))) {
		mac_addr_init(&comcerto_pfe_pdata_ds215air);

		for (i = 0; i < ARRAY_SIZE(comcerto_devices); i++){
			if(0 == strncmp(comcerto_devices[i]->name, SYNO_PFE_NAME, strlen(SYNO_PFE_NAME))) {
				comcerto_devices[i]->dev.platform_data = &comcerto_pfe_pdata_ds215air;
				break;
			}
		}
	} else {
		mac_addr_init(&comcerto_pfe_pdata);
	}

	platform_add_devices(comcerto_devices, ARRAY_SIZE(comcerto_devices));
#else
	mac_addr_init(&comcerto_pfe_pdata);

	platform_add_devices(comcerto_devices, ARRAY_SIZE(comcerto_devices));
#endif

#ifdef MY_ABC_HERE
	pm_power_off = synology_power_off;
	arm_pm_restart = synology_restart;
#endif
}

MACHINE_START(COMCERTO, "Comcerto 2000 EVM")
	 
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
