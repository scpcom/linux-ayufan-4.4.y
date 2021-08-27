#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
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
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/mbus.h>
#include <asm/mach/time.h>
#include <linux/clocksource.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <mach/system.h>

#include <linux/tty.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/ata_platform.h>
#include <asm/serial.h>
#include <plat/cache-aurora-l2.h>

#include <mach/serial.h>
#include <plat/audio.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "mvDebug.h"
#include "mvSysHwConfig.h"
#include "pex/mvPexRegs.h"
#include "cntmr/mvCntmr.h"
#include "gpp/mvGpp.h"
#include "plat/gpio.h"
#include "cpu/mvCpu.h"

#if defined(CONFIG_MV_INCLUDE_SDIO)
#include "sdmmc/mvSdmmc.h"
#include <plat/mvsdio.h>
#endif
#if defined(CONFIG_MV_INCLUDE_CESA)
#include "cesa/mvCesa.h"
#endif
#if defined(CONFIG_MV_INCLUDE_AUDIO)
#include <plat/i2s-orion.h>
#endif

#include <plat/mv_xor.h>

/* I2C */
#include <linux/i2c.h>
#include <linux/mv643xx_i2c.h>
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"

/* SPI */
#include "mvSysSpiApi.h"

/* Eth Phy */
#include "mvSysEthPhyApi.h"

/* NAND */
#ifdef CONFIG_MTD_NAND_NFC
#include "mv_mtd/nand_nfc.h"
#endif

/* DLB */
#define MV_DLB_CTRL_REG				(INTER_REGS_BASE + 0x1700)
#define MV_DLB_BUS_OPT_WEIGHTS_REG		(INTER_REGS_BASE + 0x1704)
#define MV_DLB_CMD_PRIO_REG			(INTER_REGS_BASE + 0x1708)
#define MV_MBUS_UNITS_PRIO_CTRL_REG		(INTER_REGS_BASE + 0x20420)
#define MV_FABRIC_UNITS_PRIO_CTRL_REG		(INTER_REGS_BASE + 0x20424)

#define MV_COHERENCY_FABRIC_CTRL_REG		(MV_COHERENCY_FABRIC_OFFSET + 0x0)
#define MV_COHERENCY_FABRIC_CFG_REG		(MV_COHERENCY_FABRIC_OFFSET + 0x4)

extern unsigned int irq_int_type[];
extern void __init axp_map_io(void);
extern void __init mv_init_irq(void);
extern struct sys_timer axp_timer;
extern MV_CPU_DEC_WIN* mv_sys_map(void);
#if defined(CONFIG_MV_INCLUDE_CESA)
extern u32 mv_crypto_virt_base_get(u8 chan);
#endif
extern void axp_init_irq(void);
unsigned int support_wait_for_interrupt;
u32 bit_mask_config;

/* for debug putstr */
static char arr[256];
MV_U32 mvTclk = 166666667;
MV_U32 mvSysclk = 200000000;

#ifdef CONFIG_MV_INCLUDE_GIG_ETH
MV_U8 mvMacAddr[CONFIG_MV_ETH_PORTS_NUM][6];
MV_U16 mvMtu[CONFIG_MV_ETH_PORTS_NUM] = {0};
#endif

/*
 * Helpers to get DDR bank info
 */
#define DDR_BASE_CS_OFF(n)	(0x0180 + ((n) << 2))
#define DDR_SIZE_CS_OFF(n)	(0x0184 + ((n) << 2))
#define TARGET_DDR		0
#define COHERENCY_STATUS_SHARED_NO_L2_ALLOC	0x1

struct mbus_dram_target_info armadaxp_mbus_dram_info;

/* USB Station */
extern int gSynoUSBStation;

/*********************************************************************************/
/**************                 Early Printk Support                **************/
/*********************************************************************************/
#ifdef MV_INCLUDE_EARLY_PRINTK
#define MV_UART0_LSR 	(*(volatile unsigned char *)(INTER_REGS_BASE + 0x12000 + 0x14))
#define MV_UART0_THR	(*(volatile unsigned char *)(INTER_REGS_BASE + 0x12000 + 0x0 ))
#define MV_UART1_LSR    (*(volatile unsigned char *)(INTER_REGS_BASE + 0x12100 + 0x14))
#define MV_UART1_THR    (*(volatile unsigned char *)(INTER_REGS_BASE + 0x12100 + 0x0 ))
#define MV_SERIAL_BASE 	((unsigned char *)(INTER_REGS_BASE + 0x12000 + 0x0 ))
#define DEV_REG		(*(volatile unsigned int *)(INTER_REGS_BASE + 0x40000))
#define CLK_REG         (*(volatile unsigned int *)(INTER_REGS_BASE + 0x2011c))
/*
 * This does not append a newline
 */
static void putstr(const char *s)
{
	unsigned int model;

	/* Get dev ID, make sure pex clk is on */
	if((CLK_REG & 0x4) == 0)
	{
		CLK_REG = CLK_REG | 0x4;
		model = (DEV_REG >> 16) & 0xffff;
		CLK_REG = CLK_REG & ~0x4;
	}
	else
		model = (DEV_REG >> 16) & 0xffff;

        while (*s) {
		while ((MV_UART0_LSR & UART_LSR_THRE) == 0);
		MV_UART0_THR = *s;

                if (*s == '\n') {
                        while ((MV_UART0_LSR & UART_LSR_THRE) == 0);
                        MV_UART0_THR = '\r';
                }
                s++;
        }
}
extern void putstr(const char *ptr);
void mv_early_printk(char *fmt,...)
{
	va_list args;
	va_start(args, fmt);
	vsprintf(arr,fmt,args);
	va_end(args);
	putstr(arr);
}
#endif

/*********************************************************************************/
/**************               UBoot Tagging Parameters              **************/
/*********************************************************************************/
extern MV_U32 gBoardId;
extern unsigned int elf_hwcap;
extern u32 mvIsUsbHost;

static int __init parse_tag_mv_uboot(const struct tag *tag)
{
	unsigned int mvUbootVer = 0;
	int i = 0;

	printk("Using UBoot passing parameters structure\n");
	mvUbootVer = tag->u.mv_uboot.uboot_version;
	mvIsUsbHost = tag->u.mv_uboot.isUsbHost;
	gBoardId =  (mvUbootVer & 0xff);
	bit_mask_config = tag->u.mv_uboot.bit_mask_config;

#ifdef CONFIG_MV_INCLUDE_GIG_ETH
	for (i = 0; i < CONFIG_MV_ETH_PORTS_NUM; i++) {
#if defined (CONFIG_OVERRIDE_ETH_CMDLINE)
		memset(mvMacAddr[i], 0, 6);
		mvMtu[i] = 0;
#else
printk(">>>>>>>Tag MAC %02x:%02x:%02x:%02x:%02x:%02x\n", tag->u.mv_uboot.macAddr[i][5], tag->u.mv_uboot.macAddr[i][4],
	tag->u.mv_uboot.macAddr[i][3], tag->u.mv_uboot.macAddr[i][2], tag->u.mv_uboot.macAddr[i][1], tag->u.mv_uboot.macAddr[i][0]);
		memcpy(mvMacAddr[i], tag->u.mv_uboot.macAddr[i], 6);
		mvMtu[i] = tag->u.mv_uboot.mtu[i];
#endif
	}
#endif

#ifdef CONFIG_MV_NAND
	/* get NAND ECC type(1-bit or 4-bit) */
	mv_nand_ecc = tag->u.mv_uboot.nand_ecc;
#endif
	return 0;
}

__tagtable(ATAG_MV_UBOOT, parse_tag_mv_uboot);

/*********************************************************************************/
/**************                Command Line Parameters              **************/
/*********************************************************************************/
#ifdef CONFIG_MV_INCLUDE_USB
#include "mvSysUsbApi.h"
/* Required to get the configuration string from the Kernel Command Line */
static char *usb0Mode = "host";
static char *usb1Mode = "host";
int mv_usb0_cmdline_config(char *s);
int mv_usb1_cmdline_config(char *s);
__setup("usb0Mode=", mv_usb0_cmdline_config);
__setup("usb1Mode=", mv_usb1_cmdline_config);

int mv_usb0_cmdline_config(char *s)
{
    usb0Mode = s;
    return 1;
}

int mv_usb1_cmdline_config(char *s)
{
    usb1Mode = s;
    return 1;
}
#endif

#ifdef CONFIG_CACHE_AURORA_L2
static int noL2 = 0;
static int __init noL2_setup(char *__unused)
{
     noL2 = 1;
     return 1;
}

__setup("noL2", noL2_setup);
#endif

#ifndef CONFIG_SHEEVA_ERRATA_ARM_CPU_4948
unsigned int l0_disable_flag = 0;		/* L0 Enabled by Default */
static int __init l0_disable_setup(char *__unused)
{
     l0_disable_flag = 1;
     return 1;
}

__setup("l0_disable", l0_disable_setup);
#endif

#ifndef CONFIG_SHEEVA_ERRATA_ARM_CPU_5315
unsigned int sp_enable_flag = 0;		/* SP Disabled by Default */
static int __init spec_prefesth_setup(char *__unused)
{
     sp_enable_flag = 1;
     return 1;
}

__setup("sp_enable", spec_prefesth_setup);
#endif

char *nfcConfig = NULL;
static int __init nfcConfig_setup(char *s)
{
	nfcConfig = s;
	return 1;
}
__setup("nfcConfig=", nfcConfig_setup);

static int dlb_enable = 1;
static int __init dlb_setup(char *__unused)
{
     dlb_enable = 0;
     return 1;
}

__setup("noDLB", dlb_setup);

void __init armadaxp_setup_cpu_mbus(void)
{
	void __iomem *addr;
	int i;
	int cs;
	u8 coherency_status = 0;

#if defined(CONFIG_AURORA_IO_CACHE_COHERENCY)
	coherency_status = COHERENCY_STATUS_SHARED_NO_L2_ALLOC;
#endif

	/*
	 * Setup MBUS dram target info.
	 */
	armadaxp_mbus_dram_info.mbus_dram_target_id = TARGET_DDR;
	addr = (void __iomem *)DDR_WINDOW_CPU_BASE;

	for (i = 0, cs = 0; i < 4; i++) {
		u32 base = readl(addr + DDR_BASE_CS_OFF(i));
		u32 size = readl(addr + DDR_SIZE_CS_OFF(i));

		/*
		 * Chip select enabled?
		 */
		if (size & 1) {
			struct mbus_dram_window *w;

			w = &armadaxp_mbus_dram_info.cs[cs++];
			w->cs_index = i;
			w->mbus_attr = 0xf & ~(1 << i);
			w->mbus_attr |= coherency_status << 4;
			w->base = base & 0xff000000;
			w->size = (size | 0x00ffffff) + 1;
		}
	}
	armadaxp_mbus_dram_info.num_cs = cs;
}

/*********************************************************************************/
/**************               I/O Devices Platform Info             **************/
/*********************************************************************************/
/*************
 * I2C(TWSI) *
 *************/
static struct mv64xxx_i2c_pdata axp_i2c_pdata = {
       .freq_m         = 8, /* assumes 166 MHz TCLK */
       .freq_n         = 3,
       .timeout        = 1000, /* Default timeout of 1 second */
};

static struct resource axp_i2c_0_resources[] = {
	{
		.name   = "i2c base",
		.start  = INTER_REGS_PHYS_BASE + MV_TWSI_SLAVE_REGS_OFFSET(0),
		.end    = INTER_REGS_PHYS_BASE + MV_TWSI_SLAVE_REGS_OFFSET(0) + 0x20 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name   = "i2c irq",
		.start  = IRQ_AURORA_I2C0,
		.end    = IRQ_AURORA_I2C0,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device axp_i2c = {
	.name           = MV64XXX_I2C_CTLR_NAME,
	.id             = 0,
	.num_resources  = ARRAY_SIZE(axp_i2c_0_resources),
	.resource       = axp_i2c_0_resources,
	.dev            = {
		.platform_data = &axp_i2c_pdata,
	},
};

/**********
 * UART-0 *
 **********/
static struct plat_serial8250_port aurora_uart0_data[] = {
	{
		.mapbase	= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(0)),
		.membase	= (char *)(INTER_REGS_BASE | MV_UART_REGS_OFFSET(0)),
		.irq		= IRQ_AURORA_UART0,
		.flags		= UPF_FIXED_TYPE | UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_DWAPB,
		.private_data	= (void *) (INTER_REGS_BASE | MV_UART_REGS_OFFSET(0) | 0x7C),
		.type		= PORT_16550A,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource aurora_uart0_resources[] = {
	{
		.start		= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(0)),
		.end		= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(0)) + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_AURORA_UART0,
		.end		= IRQ_AURORA_UART0,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device aurora_uart0 = {
	.name			= "serial8250",
	.id			= 0,
	.dev			= {
		.platform_data	= aurora_uart0_data,
	},
	.resource		= aurora_uart0_resources,
	.num_resources		= ARRAY_SIZE(aurora_uart0_resources),
};

#if defined(CONFIG_SYNO_ARMADA_ARCH)
/**********
 * UART-1 *
 **********/
static struct plat_serial8250_port aurora_uart1_data[] = {
	{
		.mapbase	= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(1)),
		.membase	= (char *)(INTER_REGS_BASE | MV_UART_REGS_OFFSET(1)),
		.irq		= IRQ_AURORA_UART1,
		.flags		= UPF_FIXED_TYPE | UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_DWAPB,
		.private_data	= (void *) (INTER_REGS_BASE | MV_UART_REGS_OFFSET(1) | 0x7C),
		.type		= PORT_16550A,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource aurora_uart1_resources[] = {
	{
		.start		= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(1)),
		.end		= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(1)) + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_AURORA_UART1,
		.end		= IRQ_AURORA_UART1,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device aurora_uart1 = {
	.name			= "serial8250",
	.id			= 1,
	.dev			= {
		.platform_data	= aurora_uart1_data,
	},
	.resource		= aurora_uart1_resources,
	.num_resources		= ARRAY_SIZE(aurora_uart1_resources),
};
#endif

void __init serial_initialize(void)
{
	aurora_uart0_data[0].uartclk = mvBoardTclkGet();
	platform_device_register(&aurora_uart0);
#if defined(CONFIG_SYNO_ARMADA_ARCH)
	aurora_uart1_data[0].uartclk = mvBoardTclkGet();
	platform_device_register(&aurora_uart1);
#endif
}

#ifdef CONFIG_MV_INCLUDE_AUDIO

/*****************************************************************************
 * I2S/SPDIF
 ****************************************************************************/
static struct resource mv_i2s_resources[] = {
	[0] = {
		.start	= INTER_REGS_PHYS_BASE + MV_AUDIO_REGS_OFFSET(0),
		.end	= INTER_REGS_PHYS_BASE + MV_AUDIO_REGS_OFFSET(0) + SZ_16K -1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_AURORA_AUDIO,
		.end	= IRQ_AURORA_AUDIO,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 mv_i2s0_dmamask = 0xFFFFFFFFUL;

static struct orion_i2s_platform_data mv_i2s_plat_data = {
	.dram	= NULL,
	.spdif_rec = 1,
	.spdif_play = 1,
	.i2s_rec = 1,
	.i2s_play = 1,
};

static struct platform_device mv_i2s = {
	.name           = "mv88fx_snd",
	.id             = 0,
	.dev            = {
		.dma_mask = &mv_i2s0_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
		.platform_data	= &mv_i2s_plat_data,
	},
	.num_resources  = ARRAY_SIZE(mv_i2s_resources),
	.resource       = mv_i2s_resources,
};

static struct platform_device mv_mv88fx_i2s = {
	.name           = "mv88fx-i2s",
	.id             = -1,
};

/*****************************************************************************
 * A2D on I2C bus
 ****************************************************************************/
static struct i2c_board_info __initdata i2c_a2d = {
	I2C_BOARD_INFO("i2s_i2c", 0x4A),
};

void __init mv_audio_init(void)
{
       if (MV_TRUE == mvCtrlPwrClckGet(AUDIO_UNIT_ID, 0)) {
		platform_device_register(&mv_mv88fx_i2s);
		platform_device_register(&mv_i2s);
		i2c_register_board_info(0, &i2c_a2d, 1);
       }
}

#endif /* #ifdef CONFIG_MV_INCLUDE_AUDIO */

/*****************************************************************************
 * Audio
 ****************************************************************************/
static struct resource kirkwood_i2s_resources[] = {
	[0] = {
		.start	= INTER_REGS_PHYS_BASE + MV_AUDIO_REGS_OFFSET(0),
		.end	= INTER_REGS_PHYS_BASE + MV_AUDIO_REGS_OFFSET(0) + SZ_16K -1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_AURORA_AUDIO,
		.end	= IRQ_AURORA_AUDIO,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct kirkwood_asoc_platform_data kirkwood_i2s_data = {
	.dram        = &armadaxp_mbus_dram_info,
	.burst       = 128,
};

static struct platform_device kirkwood_i2s_device = {
	.name		= "kirkwood-i2s",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(kirkwood_i2s_resources),
	.resource	= kirkwood_i2s_resources,
	.dev		= {
		.platform_data	= &kirkwood_i2s_data,
	},
};

static struct platform_device kirkwood_pcm_device = {
	.name		= "kirkwood-pcm-audio",
	.id		= -1,
};

/*****************************************************************************
 * A2D on I2C bus
 ****************************************************************************/
static struct i2c_board_info __initdata i2c_a2d = {
	I2C_BOARD_INFO("cs42l51", 0x4A),
};

void __init kirkwood_audio_init(void)
{
	if (MV_TRUE == mvCtrlPwrClckGet(AUDIO_UNIT_ID, 0)) {
		platform_device_register(&kirkwood_i2s_device);
		platform_device_register(&kirkwood_pcm_device);
		i2c_register_board_info(0, &i2c_a2d, 1);
	}
}

/************
 * GPIO
 ***********/
static struct platform_device mv_gpio = {
	.name   = "mv_gpio",
	.id             = 0,
	.num_resources  = 0,
};

static void __init mv_gpio_init(void)
{
	platform_device_register(&mv_gpio);
}

/********
 * SDIO *
 ********/
#if defined(CONFIG_MV_INCLUDE_SDIO)
static struct resource mvsdio_resources[] = {
	[0] = {
		.start	= INTER_REGS_PHYS_BASE + MV_SDMMC_REGS_OFFSET,
		.end	= INTER_REGS_PHYS_BASE + MV_SDMMC_REGS_OFFSET + SZ_1K -1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_AURORA_SDIO,
		.end	= IRQ_AURORA_SDIO,
		.flags	= IORESOURCE_IRQ,
	},

};

static u64 mvsdio_dmamask = 0xffffffffUL;

static struct mvsdio_platform_data mvsdio_data = {
	.gpio_write_protect	= 0,
	.gpio_card_detect	= 0,
	.dram			= NULL,
};

static struct platform_device mv_sdio_plat = {
	.name		= "mvsdio",
	.id		= -1,
	.dev		= {
		.dma_mask = &mvsdio_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data	= &mvsdio_data,
	},
	.num_resources	= ARRAY_SIZE(mvsdio_resources),
	.resource	= mvsdio_resources,
};
#endif /* #if defined(CONFIG_MV_INCLUDE_SDIO) */

/*******
 * GBE *
 *******/
#ifdef CONFIG_MV_ETHERNET
#if defined(CONFIG_MV_ETH_LEGACY)
static struct platform_device mv88fx_eth = {
	.name           = "mv88fx_eth",
	.id             = 0,
	.num_resources  = 0,
};
#elif defined(CONFIG_MV_ETH_NETA)
static struct platform_device mv88fx_neta = {
	.name           = "mv88fx_neta",
	.id             = 0,
	.num_resources  = 0,
};
#else
#error "Ethernet Mode is not defined (should be Legacy or NETA)"
#endif /* Ethernet mode: legacy or NETA */

static void __init eth_init(void)
{
#if defined(CONFIG_MV_ETH_LEGACY)
	platform_device_register(&mv88fx_eth);
#elif defined(CONFIG_MV_ETH_NETA)
	platform_device_register(&mv88fx_neta);
#endif /* Ethernet mode: legacy or NETA */
}
#endif /* CONFIG_MV_ETHERNET */

/*******
 * RTC *
 *******/
static struct resource axp_rtc_resource[] = {
	{
		.start	= INTER_REGS_PHYS_BASE + MV_RTC_REGS_OFFSET,
		.end	= INTER_REGS_PHYS_BASE + MV_RTC_REGS_OFFSET + 32 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_AURORA_RTC,
		.flags	= IORESOURCE_IRQ,
	}
};

static void __init rtc_init(void)
{
	platform_device_register_simple("rtc-mv", -1, axp_rtc_resource, 2);
}

/********
 * SATA *
 ********/
#ifdef CONFIG_SATA_MV
#define SATA_PHYS_BASE (INTER_REGS_PHYS_BASE | MV_SATA_REGS_OFFSET)

static struct mv_sata_platform_data dbdsmp_sata_data = {
	.n_ports	= 2,
};

static struct resource armadaxp_sata_resources[] = {
	{
		.name	= "sata base",
		.start	= SATA_PHYS_BASE,
		.end	= SATA_PHYS_BASE + 0x5000 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "sata irq",
		.start	= IRQ_AURORA_SATA(0),
		.end	= IRQ_AURORA_SATA(0),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device armadaxp_sata = {
	.name		= "sata_mv",
	.id		= 0,
	.dev		= {
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(armadaxp_sata_resources),
	.resource	= armadaxp_sata_resources,
};

void __init armadaxp_sata_init(struct mv_sata_platform_data *sata_data)
{

	/* If Port1 is disabled, then reduce the number of ports. */
	if (mvCtrlPwrClckGet(SATA_UNIT_ID, 1) == MV_FALSE)
		sata_data->n_ports--;

	/* Cannot enable port1 if port 0 is disabled. */
	if (mvCtrlPwrClckGet(SATA_UNIT_ID, 0) == MV_FALSE) {
		sata_data->n_ports = 0;
		return;
	}

	if (sata_data->n_ports != 0) {
		armadaxp_sata.dev.platform_data = sata_data;
		sata_data->dram = &armadaxp_mbus_dram_info;
		platform_device_register(&armadaxp_sata);
	}
}
#endif
/*****************************************************************************
 * SoC hwmon Thermal Sensor
 ****************************************************************************/
void __init armadaxp_hwmon_init(void)
{
	platform_device_register_simple("axp-temp", 0, NULL, 0);
}

/*************
 * 7-Segment *
 *************/
static struct timer_list axp_db_timer;
static void axp_db_7seg_event(unsigned long data)
{
	static int count = 0;

	/* Update the 7 segment */
	mvBoardDebugLed(count);

	/* Incremnt count and arm the timer*/
	count = (count + 1) & 7;
	mod_timer(&axp_db_timer, jiffies + 1 * HZ);
}

static int __init axp_db_7seg_init(void)
{
	/* Create the 7segment timer */
	setup_timer(&axp_db_timer, axp_db_7seg_event, 0);

	/* Arm it expire in 1 second */
	mod_timer(&axp_db_timer, jiffies + 1 * HZ);

	return 0;
}
__initcall(axp_db_7seg_init);

#ifdef CONFIG_MTD_NAND_NFC
/*****************************************************************************
 * NAND controller
 ****************************************************************************/
static struct resource axp_nfc_resources[] = {
	{
		.start  = INTER_REGS_BASE + MV_NFC_REGS_OFFSET,
		.end    = INTER_REGS_BASE + MV_NFC_REGS_OFFSET + 0x400 -1,
		.flags  = IORESOURCE_MEM,
	}
};

static struct mtd_partition nand_parts_info[] = {
	{
		.name		= "UBoot",
		.offset		= 0,
		.size		= 1 * SZ_1M
	},
	{
		.name		= "UImage",
		.offset	= MTDPART_OFS_APPEND,
		.size		= 4 * SZ_1M },
	{
		.name		= "Root",
		.offset	= MTDPART_OFS_APPEND,
		.size         = MTDPART_SIZ_FULL
	},
};

static struct nfc_platform_data axp_nfc_data = {
	.nfc_width	= 8,
	.num_devs	= 1,
	.num_cs		= 1,
	.use_dma	= 0,
	.ecc_type	= MV_NFC_ECC_BCH_2K,
	.parts		= nand_parts_info,
	.nr_parts	= ARRAY_SIZE(nand_parts_info),
};

static struct platform_device axp_nfc = {
	.name           = "armada-nand",
	.id             = 0,
	.dev            = {
							.platform_data = &axp_nfc_data,
						},
	.num_resources  = ARRAY_SIZE(axp_nfc_resources),
	.resource       = axp_nfc_resources,

};

static void __init axp_db_nfc_init(void)
{
	/* Check for ganaged mode */
	if (nfcConfig) {
		if (strncmp(nfcConfig, "ganged", 6) == 0) {
			axp_nfc_data.nfc_width = 16;
			axp_nfc_data.num_devs = 2;
			nfcConfig += 7;
		}

		/* Check for ECC type directive */
		if (strcmp(nfcConfig, "8bitecc") == 0) {
			axp_nfc_data.ecc_type = MV_NFC_ECC_BCH_1K;
		} else if (strcmp(nfcConfig, "12bitecc") == 0) {
			axp_nfc_data.ecc_type = MV_NFC_ECC_BCH_704B;
		} else if (strcmp(nfcConfig, "16bitecc") == 0) {
			axp_nfc_data.ecc_type = MV_NFC_ECC_BCH_512B;
		}
	}

	axp_nfc_data.tclk = mvBoardTclkGet();

	platform_device_register(&axp_nfc);
}
#endif
/*********************************************************************************/
/**************                      Helper Routines                **************/
/*********************************************************************************/
#ifdef CONFIG_MV_INCLUDE_CESA
unsigned char*  mv_sram_usage_get(int* sram_size_ptr)
{
	int used_size = 0;

#if defined(CONFIG_MV_CESA)
	used_size = sizeof(MV_CESA_SRAM_MAP);
#endif

	if(sram_size_ptr != NULL)
		*sram_size_ptr = _8K - used_size;

	return (char *)(mv_crypto_virt_base_get(0) + used_size);
}
#endif

void print_board_info(void)
{
	char name_buff[50];
	printk("\n  Marvell Armada370 Board");

	mvBoardNameGet(name_buff);
	printk("-- %s ",name_buff);

	mvCtrlModelRevNameGet(name_buff);
	printk(" Soc: %s",  name_buff);
#if defined(MV_CPU_LE)
	printk(" LE");
#else
	printk(" BE");
#endif
	printk("\n  LSP version: %s\n", LSP_VERSION);
	printk("\n\n");
	printk(" Detected Tclk %d, SysClk %d, FabricClk %d\n",mvTclk, mvSysclk, mvCpuL2ClkGet());
}

#ifdef	CONFIG_AURORA_IO_CACHE_COHERENCY
static void io_coherency_init(void)
{
	MV_U32 reg;

	/* set CIB read snoop command to ReadUnique */
	reg = MV_REG_READ(MV_CIB_CTRL_CFG_REG);
	reg &= ~(7 << 16);
	reg |= (7 << 16);
	MV_REG_WRITE(MV_CIB_CTRL_CFG_REG, reg);

        /* enable snoop CPU enable */
	MV_REG_BIT_SET(MV_COHERENCY_FABRIC_CTRL_REG, (1 << 24));
}
#endif

#ifdef CONFIG_DEBUG_LL
extern void printascii(const char *);
static void check_cpu_mode(void)
{
                u32 cpu_id_code_ext;
                int cpu_mode = 0;
                asm volatile("mrc p15, 1, %0, c15, c12, 0": "=r"(cpu_id_code_ext));

                if (((cpu_id_code_ext >> 16) & 0xF) == 0x2)
                        cpu_mode = 6;
                else if (((cpu_id_code_ext >> 16) & 0xF) == 0x3)
                        cpu_mode = 7;
                else
                        pr_err("unknow cpu mode!!!\n");
#ifdef CONFIG_DEBUGGER_MODE_V6
		if (cpu_mode != 6) {
			printascii("cpu mode (ARMv7) doesn't mach kernel configuration\n");
			panic("cpu mode mismatch");
		}
#else
#ifdef CONFIG_CPU_V7
                if (cpu_mode != 7) {
                        printascii("cpu mode (ARMv6) doesn't mach kernel configuration\n");
                        panic("cpu mode mismatch");
                }
#endif
#endif
	printk("Armada370: Working in ARMv%d mode\n",cpu_mode);
}
#endif

/*****************************************************************************
 * XOR
 ****************************************************************************/
static struct mv_xor_platform_shared_data armadaxp_xor_shared_data = {
	.dram		= &armadaxp_mbus_dram_info,
};

static u64 armadaxp_xor_dmamask = DMA_BIT_MASK(32);

/*****************************************************************************
 * XOR0
 ****************************************************************************/
static struct resource armadaxp_xor0_shared_resources[] = {
	{
		.name	= "xor 0 low",
		.start	= XOR0_PHYS_BASE,
		.end	= XOR0_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "xor 0 high",
		.start	= XOR0_HIGH_PHYS_BASE,
		.end	= XOR0_HIGH_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device armadaxp_xor0_shared = {
	.name		= MV_XOR_SHARED_NAME,
	.id		= 0,
	.dev		= {
		.platform_data = &armadaxp_xor_shared_data,
	},
	.num_resources	= ARRAY_SIZE(armadaxp_xor0_shared_resources),
	.resource	= armadaxp_xor0_shared_resources,
};

static struct resource armadaxp_xor00_resources[] = {
	[0] = {
		.start	= IRQ_AURORA_XOR0_CH0,
		.end	= IRQ_AURORA_XOR0_CH0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data armadaxp_xor00_data = {
	.shared		= &armadaxp_xor0_shared,
	.hw_id		= 0,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device armadaxp_xor00_channel = {
	.name		= MV_XOR_NAME,
	.id		= 0,
	.num_resources	= ARRAY_SIZE(armadaxp_xor00_resources),
	.resource	= armadaxp_xor00_resources,
	.dev		= {
		.dma_mask		= &armadaxp_xor_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(64),
		.platform_data		= &armadaxp_xor00_data,
	},
};

static struct resource armadaxp_xor01_resources[] = {
	[0] = {
		.start	= IRQ_AURORA_XOR0_CH1,
		.end	= IRQ_AURORA_XOR0_CH1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data armadaxp_xor01_data = {
	.shared		= &armadaxp_xor0_shared,
	.hw_id		= 1,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device armadaxp_xor01_channel = {
	.name		= MV_XOR_NAME,
	.id		= 1,
	.num_resources	= ARRAY_SIZE(armadaxp_xor01_resources),
	.resource	= armadaxp_xor01_resources,
	.dev		= {
		.dma_mask		= &armadaxp_xor_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(64),
		.platform_data		= &armadaxp_xor01_data,
	},
};

static void __init armadaxp_xor0_init(void)
{
	platform_device_register(&armadaxp_xor0_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	//dma_cap_set(DMA_MEMCPY, armadaxp_xor00_data.cap_mask);
	dma_cap_set(DMA_XOR, armadaxp_xor00_data.cap_mask);
	platform_device_register(&armadaxp_xor00_channel);

	//dma_cap_set(DMA_MEMCPY, armadaxp_xor01_data.cap_mask);
	//dma_cap_set(DMA_MEMSET, armadaxp_xor01_data.cap_mask);
	dma_cap_set(DMA_XOR, armadaxp_xor01_data.cap_mask);
	platform_device_register(&armadaxp_xor01_channel);
}

/*****************************************************************************
 * XOR1
 ****************************************************************************/
static struct resource armadaxp_xor1_shared_resources[] = {
	{
		.name	= "xor 1 low",
		.start	= XOR1_PHYS_BASE,
		.end	= XOR1_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "xor 1 high",
		.start	= XOR1_HIGH_PHYS_BASE,
		.end	= XOR1_HIGH_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device armadaxp_xor1_shared = {
	.name		= MV_XOR_SHARED_NAME,
	.id		= 1,
	.dev		= {
		.platform_data = &armadaxp_xor_shared_data,
	},
	.num_resources	= ARRAY_SIZE(armadaxp_xor1_shared_resources),
	.resource	= armadaxp_xor1_shared_resources,
};

static struct resource armadaxp_xor10_resources[] = {
	[0] = {
		.start	= IRQ_AURORA_XOR1_CH0,
		.end	= IRQ_AURORA_XOR1_CH0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data armadaxp_xor10_data = {
	.shared		= &armadaxp_xor1_shared,
	.hw_id		= 0,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device armadaxp_xor10_channel = {
	.name		= MV_XOR_NAME,
	.id		= 2,
	.num_resources	= ARRAY_SIZE(armadaxp_xor10_resources),
	.resource	= armadaxp_xor10_resources,
	.dev		= {
		.dma_mask		= &armadaxp_xor_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(64),
		.platform_data		= &armadaxp_xor10_data,
	},
};

static struct resource armadaxp_xor11_resources[] = {
	[0] = {
		.start	= IRQ_AURORA_XOR1_CH1,
		.end	= IRQ_AURORA_XOR1_CH1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data armadaxp_xor11_data = {
	.shared		= &armadaxp_xor1_shared,
	.hw_id		= 1,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device armadaxp_xor11_channel = {
	.name		= MV_XOR_NAME,
	.id		= 3,
	.num_resources	= ARRAY_SIZE(armadaxp_xor11_resources),
	.resource	= armadaxp_xor11_resources,
	.dev		= {
		.dma_mask		= &armadaxp_xor_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(64),
		.platform_data		= &armadaxp_xor11_data,
	},
};

static void __init armadaxp_xor1_init(void)
{
	platform_device_register(&armadaxp_xor1_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	dma_cap_set(DMA_MEMCPY, armadaxp_xor10_data.cap_mask);
	//dma_cap_set(DMA_XOR, armadaxp_xor10_data.cap_mask);
	platform_device_register(&armadaxp_xor10_channel);

	dma_cap_set(DMA_MEMCPY, armadaxp_xor11_data.cap_mask);
	dma_cap_set(DMA_MEMSET, armadaxp_xor11_data.cap_mask);
	//dma_cap_set(DMA_XOR, armadaxp_xor11_data.cap_mask);
	platform_device_register(&armadaxp_xor11_channel);
}

static void dram_dlb_setup(void)
{
	if (dlb_enable) {
		printk("Enable DLB and DRAM write coalescing\n");
		writel(0x9083, MV_DLB_BUS_OPT_WEIGHTS_REG);
		writel(0x250020, MV_DLB_CMD_PRIO_REG);
		writel(0x55555555, MV_MBUS_UNITS_PRIO_CTRL_REG);
		writel(0x2, MV_FABRIC_UNITS_PRIO_CTRL_REG);
		writel(0x7, MV_DLB_CTRL_REG);
	}
}

static void cpu_fabric_common_init(void)
{
	MV_U32	reg;

#ifdef CONFIG_DEBUG_LL
        check_cpu_mode();
#endif

#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_4948
	printk("L0 cache Disabled (by Errata #4948)\n");
#else
	__asm volatile ("mrc p15, 1, %0, c15, c1, 0" : "=r" (reg));
	if (l0_disable_flag) {
		printk("L0 cache Disabled\n");
		reg |= (1 << 0);
	} else {
		printk("L0 cache Enabled\n");
		reg &= ~(1 << 0);
	}
	__asm volatile ("mcr p15, 1, %0, c15, c1, 0" : : "r" (reg));
#endif

#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_5315
	printk("Speculative Prefetch Disabled (by Errata #5315)\n");
#else
	__asm volatile ("mrc p15, 1, %0, c15, c2, 0" : "=r" (reg));
	if (sp_enable_flag) {
		printk("Speculative Prefetch Enabled\n");
		reg &= ~(1 << 7);
	} else {
		printk("Speculative Prefetch Disabled\n");
		reg |= (1 << 7);
	}
	__asm volatile ("mcr p15, 1, %0, c15, c2, 0" : : "r" (reg));
#endif

#ifdef CONFIG_CACHE_AURORA_L2
	if (!noL2)
		aurora_l2_init((void __iomem *)(INTER_REGS_BASE + MV_AURORA_L2_REGS_OFFSET));
#endif

#ifdef	CONFIG_AURORA_IO_CACHE_COHERENCY
	printk("Support IO coherency.\n");
	io_coherency_init();
#endif
}

#ifdef CONFIG_SYNO_ARMADA_ARCH
#ifdef MY_ABC_HERE
extern void syno_mv_net_shutdown();
#endif
#define UART1_REG(x)                    (PORT1_BASE + ((UART_##x) << 2))
#define SET8N1                                  0x3
#define SOFTWARE_SHUTDOWN               0x31
#define SOFTWARE_REBOOT                 0x43
extern void synology_gpio_init(void);

static void synology_power_off(void)
{
#ifdef MY_ABC_HERE
	/* platform driver will not shutdown when poweroff */
	syno_mv_net_shutdown();
#endif

	if (!gSynoUSBStation) {
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_SHUTDOWN, UART1_REG(TX));
	}
}

static void synology_restart(char mode, const char *cmd)
{
	if (gSynoUSBStation) {
		mvBoardReset();
	} else {
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_REBOOT, UART1_REG(TX));
	}

	/* Calls original reset function for models those do not use uP
	* I.e. USB Station. */
	arm_machine_restart(mode, cmd);
}
#endif /* CONFIG_SYNO_ARMADA_ARCH */

/*****************************************************************************
 * DB BOARD: Main Initialization
 ****************************************************************************/
static void __init axp_db_init(void)
{
	/* Call Aurora/cpu special configurations */
	cpu_fabric_common_init();

	/* Enable DLB and DRAM write coalescing */
	dram_dlb_setup();

	/* init the Board environment */
	mvBoardEnvInit();

	/* init the controller environment */
	if( mvCtrlEnvInit() ) {
		printk( "Controller env initialization failed.\n" );
		return;
	}

	/* Set configuration according to bit mask passed from U-Boot */
	mvBoardBitMaskConfigSet(bit_mask_config);

	armadaxp_setup_cpu_mbus();

	/* Init the CPU windows setting and the access protection windows. */
	if( mvCpuIfInit(mv_sys_map())) {
		printk( "Cpu Interface initialization failed.\n" );
		return;
	}

	/* Init Tclk & SysClk */
	mvTclk = mvBoardTclkGet();
	mvSysclk = mvBoardSysClkGet();

	support_wait_for_interrupt = 1;

#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_BTS61
	support_wait_for_interrupt = 0;
#endif

	elf_hwcap &= ~HWCAP_JAVA;

	serial_initialize();

	/* At this point, the CPU windows are configured according to default definitions in mvSysHwConfig.h */
	/* and cpuAddrWinMap table in mvCpuIf.c. Now it's time to change defaults for each platform.         */
	mvCpuIfAddDecShow();

	print_board_info();

	mv_gpio_init();

	/* RTC */
	rtc_init();

	/* SPI */
	mvSysSpiInit(0, _16M);
	mvSysSpiInit(1, _16M);

#ifdef CONFIG_MV_INCLUDE_AUDIO
	/* Audio */
	mv_audio_init();
#endif
	kirkwood_audio_init();

	/* ETH-PHY */
	mvSysEthPhyInit();

	/* Sata */
#ifdef CONFIG_SATA_MV
	armadaxp_sata_init(&dbdsmp_sata_data);
#endif
#ifdef CONFIG_MTD_NAND_NFC
	/* NAND */
	axp_db_nfc_init();
#endif
	/* HWMON */
	armadaxp_hwmon_init();

	/* XOR */
	armadaxp_xor0_init();
	armadaxp_xor1_init();

	/* I2C */
	platform_device_register(&axp_i2c);

#if defined(CONFIG_MV_INCLUDE_SDIO)
	if (MV_TRUE == mvCtrlPwrClckGet(SDIO_UNIT_ID, 0)) {
		int irq_detect = mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_DETECT);
		MV_UNIT_WIN_INFO addrWinMap[MAX_TARGETS + 1];

		if (irq_detect != MV_ERROR) {
			mvsdio_data.gpio_card_detect = mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_DETECT);
			irq_int_type[mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_DETECT)+IRQ_AURORA_GPIO_START] = GPP_IRQ_TYPE_CHANGE_LEVEL;
		}

		if(mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_WP) != MV_ERROR)
			mvsdio_data.gpio_write_protect = mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_WP);

		if(MV_OK == mvCtrlAddrWinMapBuild(addrWinMap, MAX_TARGETS + 1))
			if (MV_OK == mvSdmmcWinInit(addrWinMap))
				mvsdio_data.clock = mvBoardTclkGet();
		platform_device_register(&mv_sdio_plat);
       }
#endif

#ifdef CONFIG_MV_ETHERNET
	/* Ethernet */
	eth_init();
#endif

#if defined(CONFIG_SYNO_ARMADA_ARCH)
	pm_power_off = synology_power_off;
	arm_pm_restart = synology_restart;
	synology_gpio_init();
#endif

	return;
}

MACHINE_START(ARMADA_370, "Marvell Armada-370")
	/* MAINTAINER("MARVELL") */
	.atag_offset	= 0x00000100,
	.map_io		= axp_map_io,
	.init_irq	= axp_init_irq,
	.timer		= &axp_timer,
	.init_machine	= axp_db_init,
MACHINE_END
