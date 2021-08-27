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
#include <linux/ioport.h>
#include <linux/ata_platform.h>
#include <linux/ethtool.h>
#include <linux/device.h>
#include <linux/mtd/partitions.h>
#include <linux/string.h>
#include <linux/mbus.h>
#include <linux/mv643xx_i2c.h>
#include <linux/module.h>
#include <asm/smp_scu.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <mach/system.h>

#include <linux/tty.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <asm/serial.h>

#include <mach/serial.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvUnitMap.h"
#include "cpu/mvCpu.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "mvSysHwConfig.h"

#ifdef CONFIG_MTD_NAND_NFC
#include "mv_mtd/nand_nfc.h"
#endif

#if defined(CONFIG_MV_INCLUDE_SDIO)
#include "sdmmc/mvSdmmc.h"
#include <plat/mvsdio.h>
#endif

#ifdef CONFIG_MV_INCLUDE_XOR
#include <plat/mv_xor.h>
#endif

#if defined(CONFIG_MV_ETH_PP2) || defined(CONFIG_MV_ETH_PP2_MODULE)
#include <linux/mv_pp2.h>
#endif

#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"
#include "mvSysEthPhyApi.h"

#include <asm/hardware/cache-l2x0.h>
#include <asm/hardware/gic.h>
#include "ca9x2.h"
#include "core.h"

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
extern void synology_gpio_init(void);
#endif

MV_STATUS mvSysSpiInit(MV_U8 spiId, MV_U32 serialBaudRate);

extern void __init a375_map_io(void);
extern struct sys_timer a375_timer;
extern MV_CPU_DEC_WIN *mv_sys_map(void);

extern void a375_init_irq(void);
extern void __init set_core_count(unsigned int cpu_count);
#ifdef CONFIG_SMP
extern unsigned int group_cpu_mask;
#else
static unsigned int group_cpu_mask = 1;
#endif /* CONFIG_SMP */

/* for debug putstr */
static char arr[256];

#ifdef CONFIG_MV_INCLUDE_GIG_ETH
MV_U8 mvMacAddr[MV_UBOOT_ETH_PORTS][6];
MV_U16 mvMtu[MV_UBOOT_ETH_PORTS] = { 0 };
#endif

/*
 * Helpers to get DDR bank info
 */
#define DDR_BASE_CS_OFF(n)      (0x0180 + ((n) << 3))
#define DDR_SIZE_CS_OFF(n)      (0x0184 + ((n) << 3))
#define TARGET_DDR              0
#define COHERENCY_STATUS_SHARED_NO_L2_ALLOC     0x1

struct mbus_dram_target_info a375_mbus_dram_info;

const struct mbus_dram_target_info *mv_mbus_dram_info(void)
{
	return &a375_mbus_dram_info;
}
EXPORT_SYMBOL(mv_mbus_dram_info);

/*******************************************************************************
 * Early Printk Support
 */
#ifdef MV_INCLUDE_EARLY_PRINTK
#define MV_UART0_LSR    (*(unsigned char *)(INTER_REGS_VIRT_BASE + 0x12000 + 0x14))
#define MV_UART0_THR    (*(unsigned char *)(INTER_REGS_VIRT_BASE + 0x12000 + 0x0))
/*
 * This does not append a newline
 */
static void putstr(const char *s)
{
	while (*s) {
		while ((MV_UART0_LSR & UART_LSR_THRE) == 0)
			;
		MV_UART0_THR = *s;

		if (*s == '\n') {
			while ((MV_UART0_LSR & UART_LSR_THRE) == 0)
				;
			MV_UART0_THR = '\r';
		}
		s++;
	}
}

extern void putstr(const char *ptr);
void mv_early_printk(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsprintf(arr, fmt, args);
	va_end(args);
	putstr(arr);
}
#endif /* MV_INCLUDE_EARLY_PRINTK */

/*******************************************************************************
 * UBoot Tagging Parameters
 */
#ifdef CONFIG_BE8_ON_LE
#define read_tag(a)    le32_to_cpu(a)
#define read_mtu(a)    le16_to_cpu(a)
#else
#define read_tag(a)    a
#define read_mtu(a)    a
#endif

extern unsigned int elf_hwcap;
extern u32 mvIsUsbHost;

static int __init parse_tag_mv_uboot(const struct tag *tag)
{
	unsigned int boardId = 0;
	int i = 0;

	pr_info("Using UBoot passing parameters structure\n");
	boardId = read_tag(tag->u.mv_uboot.uboot_version);
	/* uboot_version:
	 * - Board Id is Passed in the lower byte
	 * - Used to set board ID & structure pointer - must be set before any mvBoardIdGet usage
	 */
	mvBoardSet(boardId & 0xff);

#ifdef CONFIG_MV_INCLUDE_USB
	mvIsUsbHost = read_tag(tag->u.mv_uboot.isUsbHost);
#endif

#ifdef CONFIG_MV_INCLUDE_GIG_ETH
	for (i = 0; i < MV_UBOOT_ETH_PORTS; i++) {
		memcpy(mvMacAddr[i], tag->u.mv_uboot.macAddr[i], 6);
		mvMtu[i] = read_mtu(tag->u.mv_uboot.mtu[i]);
	}
#endif
	return 0;
}

__tagtable(ATAG_MV_UBOOT, parse_tag_mv_uboot);

/*******************************************************************************
 * Command Line Parameters
 */
#ifdef CONFIG_SMP
static int __init mv_rsrc_setup(char *s)
{
	char *rsrc = strchr(s, ' ');

	/* Verify NULL termination */
	if (rsrc)
		(*rsrc) = '\0';

	/* Parse string to table */
	if (mvUnitMapSetup(s, strstr) == MV_FALSE)
		pr_err("Invalid resource string %s\n", s);

	/* Change to rsrc limited mode */
	mvUnitMapSetRsrcLimited(MV_TRUE);

	return 1;
}

__setup("mv_rsrc=", mv_rsrc_setup);
#endif

char *nfcConfig;
static int __init nfcConfig_setup(char *s)
{
	nfcConfig = s;
	return 1;
}

__setup("nfcConfig=", nfcConfig_setup);

static void __init a375_init_cpu_mbus(void)
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
	a375_mbus_dram_info.mbus_dram_target_id = TARGET_DDR;
	addr = (void __iomem *)BRIDGE_VIRT_BASE;

	for (i = 0, cs = 0; i < 4; i++) {
		u32 base = readl(addr + DDR_BASE_CS_OFF(i));
		u32 size = readl(addr + DDR_SIZE_CS_OFF(i));

		/*
		 * Chip select enabled?
		 */
		if (size & 1) {
			struct mbus_dram_window *w;
			if (base & 0xf)
				/* BaseExtension is used (> 4GB). */
				continue;
			w = &a375_mbus_dram_info.cs[cs++];
			w->cs_index = i;
			w->mbus_attr = 0xf & ~(1 << i);
			w->mbus_attr |= coherency_status << 4;
			w->base = base & 0xff000000;
			w->size = (size | 0x00ffffff) + 1;
		}
	}
	a375_mbus_dram_info.num_cs = cs;
}

#if defined(CONFIG_MV_INCLUDE_CESA)
#include "cesa/mvCesa.h"
extern u32 mv_crypto_virt_base_get(u8 chan);
#endif

#ifdef CONFIG_MV_INCLUDE_CESA
unsigned char *mv_sram_usage_get(int *sram_size_ptr)
{
	int used_size = 0;

#if defined(CONFIG_MV_CESA)
	used_size = sizeof(MV_CESA_SRAM_MAP);
#endif

	if (sram_size_ptr != NULL)
		*sram_size_ptr = _8K - used_size;

	return (char *)(mv_crypto_virt_base_get(0) + used_size);
}
#endif

/*******************************************************************************
 * I2C (TWSI)
 */
#ifdef CONFIG_I2C_MV64XXX
static struct mv64xxx_i2c_pdata a375_i2c_pdata = {
	.freq_m = 8, /* assumes 166 MHz TCLK */
	.freq_n = 3,
	.timeout = 1000, /* Default timeout of 1 second */
};

static struct resource a375_i2c_0_resources[] = {
	{
		.name = "i2c base",
		.start = INTER_REGS_PHYS_BASE + MV_TWSI_SLAVE_REGS_OFFSET(0),
		.end = INTER_REGS_PHYS_BASE + MV_TWSI_SLAVE_REGS_OFFSET(0) + 0x20 - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "i2c irq",
		.start = IRQ_GLOBAL_I2C0,
		.end = IRQ_GLOBAL_I2C0,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device a375_i2c0 = {
	.name = MV64XXX_I2C_CTLR_NAME,
	.id = 0,
	.num_resources = ARRAY_SIZE(a375_i2c_0_resources),
	.resource = a375_i2c_0_resources,
	.dev = {
		.platform_data = &a375_i2c_pdata,
	},
};

static struct resource a375_i2c_1_resources[] = {
	{
		.name = "i2c base",
		.start = INTER_REGS_PHYS_BASE + MV_TWSI_SLAVE_REGS_OFFSET(1),
		.end = INTER_REGS_PHYS_BASE + MV_TWSI_SLAVE_REGS_OFFSET(1) + 0x20 - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "i2c irq",
		.start = IRQ_GLOBAL_I2C1,
		.end = IRQ_GLOBAL_I2C1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device a375_i2c1 = {
	.name = MV64XXX_I2C_CTLR_NAME,
	.id = 1,
	.num_resources = ARRAY_SIZE(a375_i2c_1_resources),
	.resource = a375_i2c_1_resources,
	.dev = {
		.platform_data = &a375_i2c_pdata,
	},
};
#endif

static void __init a375_i2c_init(void)
{
#ifdef CONFIG_I2C_MV64XXX
	if (mvUnitMapIsMine(I2C0) == MV_TRUE)
		platform_device_register(&a375_i2c0);

	if (mvCtrlSocUnitInfoNumGet(I2C_UNIT_ID) >= 1 &&
	    mvUnitMapIsMine(I2C1) == MV_TRUE)
		platform_device_register(&a375_i2c1);
#endif
}

/**********
 * UART-0 *
 **********/
static struct plat_serial8250_port uart0_data[] = {
	{
		.mapbase	= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(0)),
		.membase	= (char *)(INTER_REGS_BASE | MV_UART_REGS_OFFSET(0)),
		.irq		= IRQ_GLOBAL_UART0,
		.flags		= UPF_FIXED_TYPE | UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_DWAPB,
		.private_data	= (void *) (INTER_REGS_BASE | MV_UART_REGS_OFFSET(0) | 0x7C),
		.type		= PORT_16550A,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource uart0_resources[] = {
	{
		.start		= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(0)),
		.end		= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(0)) + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_GLOBAL_UART0,
		.end		= IRQ_GLOBAL_UART0,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device uart0 = {
	.name			= "serial8250",
	.id			= 0,
	.dev			= {
		.platform_data	= uart0_data,
	},
	.resource		= uart0_resources,
	.num_resources		= ARRAY_SIZE(uart0_resources),
};

/**********
 * UART-1 *
 **********/
static struct plat_serial8250_port uart1_data[] = {
	{
		.mapbase	= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(1)),
		.membase	= (char *)(INTER_REGS_BASE | MV_UART_REGS_OFFSET(1)),
		.irq		= IRQ_GLOBAL_UART1,
		.flags		= UPF_FIXED_TYPE | UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_DWAPB,
		.private_data	= (void *) (INTER_REGS_BASE | MV_UART_REGS_OFFSET(1) | 0x7C),
		.type		= PORT_16550A,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource uart1_resources[] = {
	{
		.start		= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(1)),
		.end		= (INTER_REGS_PHYS_BASE | MV_UART_REGS_OFFSET(1)) + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_GLOBAL_UART1,
		.end		= IRQ_GLOBAL_UART1,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device uart1 = {
	.name			= "serial8250",
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
	.id 		= 1,
#else
	.id 		= 0,
#endif
	.dev			= {
		.platform_data	= uart1_data,
	},
	.resource		= uart1_resources,
	.num_resources		= ARRAY_SIZE(uart1_resources),
};

void __init serial_initialize(int port)
{
	if (port == 0) {
		if (mvUnitMapIsMine(UART0) == MV_FALSE) {
			printk(KERN_WARNING "uart%d resource not allocated but CONFIG_MV_UART_PORT = %d\n", port, port);
			mvUnitMapSetMine(UART0);
		}

		uart0_data[0].uartclk = mvBoardTclkGet();
		platform_device_register(&uart0);
	} else {
		if (mvUnitMapIsMine(UART1) == MV_FALSE) {
			printk(KERN_WARNING "uart%d resource not allocated but CONFIG_MV_UART_PORT = %d\n", port, port);
			mvUnitMapSetMine(UART1);
		}

		uart1_data[0].uartclk = mvBoardTclkGet();
		platform_device_register(&uart1);
	}
}

/*******************************************************************************
 * SDIO
 */
#if defined(CONFIG_MV_INCLUDE_SDIO)
static struct resource mvsdio_resources[] = {
	[0] = {
	       .start = INTER_REGS_PHYS_BASE + MV_SDMMC_REGS_OFFSET,
	       .end = INTER_REGS_PHYS_BASE + MV_SDMMC_REGS_OFFSET + SZ_1K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IRQ_GLOBAL_SDIO,
	       .end = IRQ_GLOBAL_SDIO,
	       .flags = IORESOURCE_IRQ,
	       },

};

static u64 mvsdio_dmamask = 0xffffffffUL;

static struct mvsdio_platform_data mvsdio_data = {
	.gpio_write_protect = 0,
	.gpio_card_detect = 0,
	.dram = NULL,
};

static struct platform_device mv_sdio_plat = {
	.name = "mvsdio",
	.id = -1,
	.dev = {
		.dma_mask = &mvsdio_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data = &mvsdio_data,
		},
	.num_resources = ARRAY_SIZE(mvsdio_resources),
	.resource = mvsdio_resources,
};
#endif

void __init a375_sdio_init(void)
{
#ifdef CONFIG_MV_INCLUDE_SDIO
	if (mvUnitMapIsMine(SDIO) != MV_TRUE)
		return;

	if (MV_TRUE == mvCtrlPwrClckGet(SDIO_UNIT_ID, 0)) {
		int irq_detect = mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_DETECT);
		static MV_UNIT_WIN_INFO addrWinMap[MAX_TARGETS + 1];

		if (irq_detect != MV_ERROR) {
			mvsdio_data.gpio_card_detect =
			    mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_DETECT);
		}

		if (mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_WP) != MV_ERROR)
			mvsdio_data.gpio_write_protect =
			    mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_WP);

		if (MV_OK == mvCtrlAddrWinMapBuild(addrWinMap, MAX_TARGETS + 1))
			if (MV_OK == mvSdmmcWinInit(addrWinMap))
				mvsdio_data.clock = mvBoardTclkGet();
		platform_device_register(&mv_sdio_plat);
	}
#endif
}

/*******************************************************************************
 * USB
 */

void __init a375_usb_init(void)
{
#ifdef CONFIG_MV_INCLUDE_USB
	mv_usb_init(&a375_mbus_dram_info);
#endif
}

/*******************************************************************************
 * GBE
 */
#ifdef CONFIG_MV_ETHERNET
#if defined(CONFIG_MV_ETH_PP2) || defined(CONFIG_MV_ETH_PP2_MODULE)
static void mv_pp2_giga_pdev_register(struct platform_device *pdev)
{
	struct mv_pp2_pdata *plat_data =
	    (struct mv_pp2_pdata *)pdev->dev.platform_data;
	int speed, port = pdev->id;

	plat_data->max_port = mvCtrlEthMaxPortGet();
	plat_data->tclk = mvBoardTclkGet();
	plat_data->ctrl_model = mvCtrlModelGet();
	plat_data->ctrl_rev = mvCtrlRevGet();
	plat_data->cpu_mask = group_cpu_mask;
	plat_data->phy_addr = mvBoardPhyAddrGet(port);
	plat_data->duplex = DUPLEX_FULL;

	if (mvBoardIsPortLoopback(port))
		plat_data->flags |= MV_PP2_PDATA_F_LB;

	if (mvBoardIsPortInSgmii(port))
		plat_data->flags |= MV_PP2_PDATA_F_SGMII;
	else
		plat_data->flags &= ~MV_PP2_PDATA_F_SGMII;

	if (port < MV_UBOOT_ETH_PORTS) {
		plat_data->mtu = mvMtu[port];
		if (plat_data->mtu == 0) {
			plat_data->mtu = 1500;
			pr_err
			    ("%s: warning - failed to use MTU from uboot env, changing to default MTU (1500).\n",
			     __func__);
		}

		memcpy(plat_data->mac_addr, mvMacAddr[port], 6);
	} else {
		plat_data->mtu = 1500;
		memset(plat_data->mac_addr, 0, 6);
	}

	speed = mvBoardMacSpeedGet(port);
	switch (speed) {
	case BOARD_MAC_SPEED_10M:
		plat_data->speed = SPEED_10;
		break;
	case BOARD_MAC_SPEED_100M:
		plat_data->speed = SPEED_100;
		break;
	case BOARD_MAC_SPEED_1000M:
		plat_data->speed = SPEED_1000;
		break;
	case BOARD_MAC_SPEED_AUTO:
	default:
		plat_data->speed = 0;
		break;
	}

	platform_device_register(pdev);
}

static struct resource mv_pp2_ge0_resources[] = {
	{
	 .start = IRQ_GLOBAL_PP_PORT0_RXTX,
	 .end = IRQ_GLOBAL_PP_PORT0_RXTX,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct mv_pp2_pdata mv_pp2_ge0_pdata = {
	.mtu = 1500,
	.phy_addr = 0,
	.flags = MV_PP2_PDATA_F_LINUX_CONNECT,
};

static struct platform_device mv_pp2_ge0_plat = {
	.name = MV_PP2_PORT_NAME,
	.id = 0,
	.num_resources = ARRAY_SIZE(mv_pp2_ge0_resources),
	.resource = mv_pp2_ge0_resources,
	.dev = {
		.platform_data = &mv_pp2_ge0_pdata,
		},
};

static struct resource mv_pp2_ge1_resources[] = {
	{
	 .start = IRQ_GLOBAL_PP_PORT1_RXTX,
	 .end = IRQ_GLOBAL_PP_PORT1_RXTX,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct mv_pp2_pdata mv_pp2_ge1_pdata = {
	.mtu = 1500,
	.phy_addr = 0,
	.flags = MV_PP2_PDATA_F_LINUX_CONNECT,
};

static struct platform_device mv_pp2_ge1_plat = {
	.name = MV_PP2_PORT_NAME,
	.id = 1,
	.num_resources = ARRAY_SIZE(mv_pp2_ge1_resources),
	.resource = mv_pp2_ge1_resources,
	.dev = {
		.platform_data = &mv_pp2_ge1_pdata,
		},
};

static struct resource mv_pp2_ge2_resources[] = {
	{
	 .start = IRQ_GLOBAL_PP_PORT2_RXTX,
	 .end = IRQ_GLOBAL_PP_PORT2_RXTX,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct mv_pp2_pdata mv_pp2_ge2_pdata = {
	.mtu = 1500,
	.phy_addr = -1,
	.flags = MV_PP2_PDATA_F_LINUX_CONNECT,
};

static struct platform_device mv_pp2_ge2_plat = {
	.name = MV_PP2_PORT_NAME,
	.id = 2,
	.num_resources = ARRAY_SIZE(mv_pp2_ge2_resources),
	.resource = mv_pp2_ge2_resources,
	.dev = {
		.platform_data = &mv_pp2_ge2_pdata,
		},
};

static struct mv_pp2_pdata mv_pp2_ge3_pdata = {
	.mtu = 1500,
	.phy_addr = -1,
	.flags = MV_PP2_PDATA_F_LINUX_CONNECT,
};

static struct resource mv_pp2_ge3_resources[] = {
	{
	 .start = IRQ_GLOBAL_PP_PORT7_RXTX,
	 .end = IRQ_GLOBAL_PP_PORT7_RXTX,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device mv_pp2_ge3_plat = {
	.name = MV_PP2_PORT_NAME,
	.id = 3,
	.num_resources = ARRAY_SIZE(mv_pp2_ge3_resources),
	.resource = mv_pp2_ge3_resources,
	.dev = {
		.platform_data = &mv_pp2_ge3_pdata,
		},
};
#else
#error "Ethernet Mode is not defined"
#endif /* Ethernet mode: PPv2 */

static void __init eth_init(void)
{
#if !defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	if ((mvBoardIsEthConnected(0) == MV_TRUE) && (mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, 0)))
		mv_pp2_giga_pdev_register(&mv_pp2_ge0_plat);
#endif

	if ((mvBoardIsEthConnected(1) == MV_TRUE) && (mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, 1)))
		mv_pp2_giga_pdev_register(&mv_pp2_ge1_plat);

#ifdef CONFIG_MV_INCLUDE_PON
	mv_pp2_giga_pdev_register(&mv_pp2_ge3_plat);
#endif
}

#endif /* CONFIG_MV_ETHERNET */

static void a375_init_eth(void)
{
#ifdef CONFIG_MV_ETHERNET
	mvSysEthPhyInit();
	eth_init();
#endif
}

/*******************************************************************************
 * GPIO
 */
static struct platform_device mv_gpio = {
	.name = "mv_gpio",
	.id = 0,
	.num_resources = 0,
};

static void __init a375_gpio_init(void)
{
	platform_device_register(&mv_gpio);
}

/*******************************************************************************
 * RTC
 */
static struct resource rtc_resource[] = {
	{
	 .start = INTER_REGS_PHYS_BASE + MV_RTC_REGS_OFFSET,
	 .end = INTER_REGS_PHYS_BASE + MV_RTC_REGS_OFFSET + 32 - 1,
	 .flags = IORESOURCE_MEM,
	 }, {
	     .start = IRQ_GLOBAL_RTC,
	     .flags = IORESOURCE_IRQ,
	     }
};

static void __init a375_rtc_init(void)
{
	platform_device_register_simple("rtc-mv", -1, rtc_resource, 2);
}

/*******************************************************************************
 * SATA
 */
#if defined(CONFIG_SATA_MV) || defined(CONFIG_SATA_MV_MODULE)
#define SATA_PHYS_BASE (INTER_REGS_PHYS_BASE | 0xA0000)

static struct mv_sata_platform_data a375_sata_data = {
	.n_ports = 2,
};

static struct resource a375_sata_resources[] = {
	{
	 .name = "sata base",
	 .start = SATA_PHYS_BASE,
	 .end = SATA_PHYS_BASE + 0x5000 - 1,
	 .flags = IORESOURCE_MEM,
	 }, {
	     .name = "sata irq",
	     .start = IRQ_GLOBAL_SATA0,
	     .end = IRQ_GLOBAL_SATA0,
	     .flags = IORESOURCE_IRQ,
	     },
};

static struct platform_device a375_sata = {
	.name = "sata_mv",
	.id = 0,
	.dev = {
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = ARRAY_SIZE(a375_sata_resources),
	.resource = a375_sata_resources,
};
#endif

static void __init a375_sata_init(struct mv_sata_platform_data *sata_data)
{
#if defined(CONFIG_SATA_MV) || defined(CONFIG_SATA_MV_MODULE)
	if (mvUnitMapIsMine(SATA) != MV_TRUE)
		return;

	a375_sata.dev.platform_data = sata_data;
	sata_data->dram = &a375_mbus_dram_info;
	platform_device_register(&a375_sata);
#endif
}

/*******************************************************************************
 * SoC hwmon Thermal Sensor
 */
static void __init a375_hwmon_init(void)
{
	if (mvUnitMapIsMine(HWMON) == MV_TRUE)
		platform_device_register_simple("a375-temp", 0, NULL, 0);
}

/*******************************************************************************
 * NAND controller
 */
#ifdef CONFIG_MTD_NAND_NFC
static struct resource a375_nfc_resources[] = {
	{
	 .start = INTER_REGS_PHYS_BASE + MV_NFC_REGS_OFFSET,
	 .end = INTER_REGS_PHYS_BASE + MV_NFC_REGS_OFFSET + 0x400 - 1,
	 .flags = IORESOURCE_MEM,
	 }
};

static struct mtd_partition nand_parts_info[] = {
	{
	 .name = "UBoot",
	 .offset = 0,
	 .size = 6 * SZ_1M}, {
			      .name = "UImage",
			      .offset = MTDPART_OFS_APPEND,
			      .size = 4 * SZ_1M}, {
						   .name = "Root",
						   .offset = MTDPART_OFS_APPEND,
						   .size = MTDPART_SIZ_FULL},
};

static struct nfc_platform_data a375_nfc_data = {
	.nfc_width = 8,
	.num_devs = 1,
	.num_cs = 1,
	.use_dma = 0,
	.ecc_type = MV_NFC_ECC_BCH_2K,
	.parts = nand_parts_info,
	.nr_parts = ARRAY_SIZE(nand_parts_info),
};

static struct platform_device a375_nfc = {
	.name = "armada-nand",
	.id = 0,
	.dev = {
		.platform_data = &a375_nfc_data,
		},
	.num_resources = ARRAY_SIZE(a375_nfc_resources),
	.resource = a375_nfc_resources,
};
#endif

static void __init a375_nand_nfc_init(void)
{
#ifdef CONFIG_MTD_NAND_NFC
	if (mvUnitMapIsMine(NAND) != MV_TRUE)
		return;

	/* Check for ganaged mode */
	if (nfcConfig) {
		if (strncmp(nfcConfig, "ganged", 6) == 0) {
			a375_nfc_data.nfc_width = 16;
			a375_nfc_data.num_devs = 2;
			nfcConfig += 7;
		}

		/* Check for ECC type directive */
		if (strcmp(nfcConfig, "8bitecc") == 0)
			a375_nfc_data.ecc_type = MV_NFC_ECC_BCH_1K;
		else if (strcmp(nfcConfig, "12bitecc") == 0)
			a375_nfc_data.ecc_type = MV_NFC_ECC_BCH_704B;
		else if (strcmp(nfcConfig, "16bitecc") == 0)
			a375_nfc_data.ecc_type = MV_NFC_ECC_BCH_512B;
	}

	a375_nfc_data.tclk = mvBoardTclkGet();

	platform_device_register(&a375_nfc);
#endif
}

/*******************************************************************************
 * XOR
 */
#ifdef CONFIG_MV_INCLUDE_XOR
static struct mv_xor_platform_shared_data a375_xor_shared_data = {
	.dram = &a375_mbus_dram_info,
};

static u64 a375_xor_dmamask = DMA_BIT_MASK(32);

/*
 * XOR0
 */
static struct resource a375_xor0_shared_resources[] = {
	{
	 .name = "xor 0 low",
	 .start = XOR0_PHYS_BASE,
	 .end = XOR0_PHYS_BASE + 0xff,
	 .flags = IORESOURCE_MEM,
	 }, {
	     .name = "xor 0 high",
	     .start = XOR0_HIGH_PHYS_BASE,
	     .end = XOR0_HIGH_PHYS_BASE + 0xff,
	     .flags = IORESOURCE_MEM,
	     },
};

static struct platform_device a375_xor0_shared = {
	.name = MV_XOR_SHARED_NAME,
	.id = 0,
	.dev = {
		.platform_data = &a375_xor_shared_data,
		},
	.num_resources = ARRAY_SIZE(a375_xor0_shared_resources),
	.resource = a375_xor0_shared_resources,
};

static struct resource a375_xor00_resources[] = {
	[0] = {
	       .start = IRQ_GLOBAL_XOR0_CHAN0,
	       .end = IRQ_GLOBAL_XOR0_CHAN0,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mv_xor_platform_data a375_xor00_data = {
	.shared = &a375_xor0_shared,
	.hw_id = 0,
	.pool_size = PAGE_SIZE,
};

static struct platform_device a375_xor00_channel = {
	.name = MV_XOR_NAME,
	.id = 0,
	.num_resources = ARRAY_SIZE(a375_xor00_resources),
	.resource = a375_xor00_resources,
	.dev = {
		.dma_mask = &a375_xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a375_xor00_data,
		},
};

static struct resource a375_xor01_resources[] = {
	[0] = {
	       .start = IRQ_GLOBAL_XOR0_CHAN1,
	       .end = IRQ_GLOBAL_XOR0_CHAN1,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mv_xor_platform_data a375_xor01_data = {
	.shared = &a375_xor0_shared,
	.hw_id = 1,
	.pool_size = PAGE_SIZE,
};

static struct platform_device a375_xor01_channel = {
	.name = MV_XOR_NAME,
	.id = 1,
	.num_resources = ARRAY_SIZE(a375_xor01_resources),
	.resource = a375_xor01_resources,
	.dev = {
		.dma_mask = &a375_xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a375_xor01_data,
		},
};

static void __init a375_xor0_init(void)
{
	if (mvUnitMapIsMine(XOR0) != MV_TRUE)
		return;

	platform_device_register(&a375_xor0_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	dma_cap_set(DMA_MEMCPY, a375_xor00_data.cap_mask);
	dma_cap_set(DMA_XOR, a375_xor00_data.cap_mask);
	platform_device_register(&a375_xor00_channel);

	dma_cap_set(DMA_MEMCPY, a375_xor01_data.cap_mask);
	dma_cap_set(DMA_MEMSET, a375_xor01_data.cap_mask);
	dma_cap_set(DMA_XOR, a375_xor01_data.cap_mask);
	platform_device_register(&a375_xor01_channel);
}

/*
 * XOR1
 */
static struct resource a375_xor1_shared_resources[] = {
	{
	 .name = "xor 1 low",
	 .start = XOR1_PHYS_BASE,
	 .end = XOR1_PHYS_BASE + 0xff,
	 .flags = IORESOURCE_MEM,
	 }, {
	     .name = "xor 1 high",
	     .start = XOR1_HIGH_PHYS_BASE,
	     .end = XOR1_HIGH_PHYS_BASE + 0xff,
	     .flags = IORESOURCE_MEM,
	     },
};

static struct platform_device a375_xor1_shared = {
	.name = MV_XOR_SHARED_NAME,
	.id = 1,
	.dev = {
		.platform_data = &a375_xor_shared_data,
		},
	.num_resources = ARRAY_SIZE(a375_xor1_shared_resources),
	.resource = a375_xor1_shared_resources,
};

static struct resource a375_xor10_resources[] = {
	[0] = {
	       .start = IRQ_GLOBAL_XOR1_CHAN0,
	       .end = IRQ_GLOBAL_XOR1_CHAN0,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mv_xor_platform_data a375_xor10_data = {
	.shared = &a375_xor1_shared,
	.hw_id = 0,
	.pool_size = PAGE_SIZE,
};

static struct platform_device a375_xor10_channel = {
	.name = MV_XOR_NAME,
	.id = 2,
	.num_resources = ARRAY_SIZE(a375_xor10_resources),
	.resource = a375_xor10_resources,
	.dev = {
		.dma_mask = &a375_xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a375_xor10_data,
		},
};

static struct resource a375_xor11_resources[] = {
	[0] = {
	       .start = IRQ_GLOBAL_XOR1_CHAN1,
	       .end = IRQ_GLOBAL_XOR1_CHAN1,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mv_xor_platform_data a375_xor11_data = {
	.shared = &a375_xor1_shared,
	.hw_id = 1,
	.pool_size = PAGE_SIZE,
};

static struct platform_device a375_xor11_channel = {
	.name = MV_XOR_NAME,
	.id = 3,
	.num_resources = ARRAY_SIZE(a375_xor11_resources),
	.resource = a375_xor11_resources,
	.dev = {
		.dma_mask = &a375_xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a375_xor11_data,
		},
};

static void __init a375_xor1_init(void)
{
	if (mvUnitMapIsMine(XOR1) != MV_TRUE)
		return;

	platform_device_register(&a375_xor1_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	dma_cap_set(DMA_XOR, a375_xor10_data.cap_mask);
	platform_device_register(&a375_xor10_channel);

	dma_cap_set(DMA_MEMCPY, a375_xor11_data.cap_mask);
	dma_cap_set(DMA_MEMSET, a375_xor11_data.cap_mask);
	platform_device_register(&a375_xor11_channel);
}
#endif

static void __init a375_xor_init(void)
{
#ifdef CONFIG_MV_INCLUDE_XOR
	a375_xor0_init();
	a375_xor1_init();
#endif
}

/*******************************************************************************
 * SPI
 */
static void a375_spi_init(void)
{
#ifdef CONFIG_MV_INCLUDE_SPI
	/* SPI */
	if (mvUnitMapIsMine(SPI) == MV_TRUE)
		mvSysSpiInit(0, _16M);
#endif
}

/*******************************************************************************
 * Helper Routines
 */

static void print_board_info(void)
{
	char name_buff[50];

	pr_info("\n  Marvell Armada-375");

	mvBoardNameGet(name_buff, 50);
	pr_info(" %s Board - ", name_buff);

	mvCtrlModelRevNameGet(name_buff);
	pr_info(" Soc: %s", name_buff);
#if defined(MV_CPU_LE)
	pr_info(" LE\n");
#else
	pr_info(" BE\n");
#endif
	pr_info("  LSP version: %s\n", LSP_VERSION);
#ifdef CONFIG_AURORA_IO_CACHE_COHERENCY
	pr_info("  IOCC: Support IO coherency.\n");
#ifdef CONFIG_ARMADA_IOCC_SYNC_BARRIER_WA
	pr_info("     -> Sync Barrier WA enabled\n");
#endif
#endif
#ifdef CONFIG_MV_AMP_ENABLE
	mvUnitMapPrint();
#endif
	pr_info("\n");
}

#ifdef CONFIG_DEBUG_LL
extern void printascii(const char *);
#endif

extern MV_TARGET_ATTRIB mvTargetDefaultsArray[];

/*******************************************************************************
 * SMP Init boot window
 * Note: This function is called before PUnit IO windows are configured.
 */
#if defined(CONFIG_SMP)
void __init a375_smp_secondary_boot_win_set(void)
{
	MV_AHB_TO_MBUS_DEC_WIN mbus_win;
	u32 code_len, win_num, win_phys_base = 0xFFFF0000;
	MV_TARGET target;
	void __iomem *sram_virt_base;

	if (mvCtrlRevGet() <= MV_88F6720_Z3_ID)
		target                  = CRYPT0_ENG;
	else
		target                  = BOOT_ROM_CS;

	mbus_win.target                 = target;
	mbus_win.addrWin.baseLow        = win_phys_base;
	mbus_win.addrWin.baseHigh	= 0x0;
	mbus_win.addrWin.size		= SZ_64K;
	mbus_win.enable			= MV_TRUE;

	if (mvAhbToMbusWinNumByTargetGet(target, &win_num) != MV_OK) {
		/* If not found, replace window #10.
		** Worst case this will override another window which will be
		** later fixed by mvCpuIfInit(). */
		win_num = 10;
	}

	if (mvAhbToMbusWinSet(win_num, &mbus_win) != MV_OK) {
		pr_err("%s: Error: mvAhbToMbusWinSet(win_num = %d) failed\n",
		       __func__, win_num);
		return;
	}

	if (mvCtrlRevGet() <= MV_88F6720_Z3_ID) {
		sram_virt_base = ioremap(win_phys_base, SZ_64K);

		code_len = 4 * (&a375_smp_cpu1_enable_code_end - &a375_smp_cpu1_enable_code_start);
		memcpy(sram_virt_base, &a375_smp_cpu1_enable_code_start, code_len);
	}
}
#endif

/*******************************************************************************
 * IOCC sync implementation
 */
#ifdef CONFIG_AURORA_IO_CACHE_COHERENCY

/*
 * All combinations of IOCC/SMP/UP should be supported:
 *     UP  + HWCC (Hardware Cache Coherency)
 *     UP  + SWCC (Software Cache Coherency)
 *     SMP + HWCC
 *     SMP + SWCC
 */
static void __init a375_init_iocc(void)
{

#if !defined(CONFIG_SMP)
	void __iomem *scu_base = (void __iomem *)(INTER_REGS_VIRT_BASE + A9_MPCORE_SCU);
	scu_enable(scu_base);
#endif

}
#endif /* CONFIG_AURORA_IO_CACHE_COHERENCY */

static void __init a375_init_l2x0_cache(void)
{
#ifdef CONFIG_CACHE_L2X0
	void __iomem *l2x0_base =
	    (void __iomem *)(INTER_REGS_VIRT_BASE + MV_CA9X2_L2CC_OFFSET);
	l2x0_init(l2x0_base, 0x00400000, 0xfe0fffff);
#ifdef CONFIG_L2X0_INSTRUCTION_ONLY
	int i;
	for (i = 0; i < 8; i++)
		writel_relaxed(0xffff, l2x0_base + L2X0_LOCKDOWN_WAY_D_BASE + i * L2X0_LOCKDOWN_STRIDE);

	outer_flush_all();
#endif
#endif
}

static inline int is_a375_pcie_phys_memory(u32 addr)
{
	switch (mvCtrlSocUnitInfoNumGet(PEX_UNIT_ID)) {
	case 0:
		return 0;
	case 1:
		return ((addr >= PEX0_MEM_PHYS_BASE) && (addr < (PEX0_MEM_PHYS_BASE + PEX0_MEM_SIZE)));
	case 2:
		return (((addr >= PEX0_MEM_PHYS_BASE) && (addr < (PEX0_MEM_PHYS_BASE + PEX0_MEM_SIZE))) || \
			((addr >= PEX1_MEM_PHYS_BASE) && (addr < (PEX1_MEM_PHYS_BASE + PEX1_MEM_SIZE))));
	default:
		pr_err("%s: Error: invalid number of PCIe ports\n", __func__);
		return 0;
	}
}

static inline int is_a375_pcie_virt_memory(u32 addr)
{
	switch (mvCtrlSocUnitInfoNumGet(PEX_UNIT_ID)) {
	case 0:
		return 0;
	case 1:
		return ((addr >= PEX0_MEM_VIRT_BASE) && (addr < (PEX0_MEM_VIRT_BASE + PEX0_MEM_SIZE)));
	case 2:
		return (((addr >= PEX0_MEM_VIRT_BASE) && (addr < (PEX0_MEM_VIRT_BASE + PEX0_MEM_SIZE))) || \
			((addr >= PEX1_MEM_VIRT_BASE) && (addr < (PEX1_MEM_VIRT_BASE + PEX1_MEM_SIZE))));
	default:
		pr_err("%s: Error: invalid number of PCIe ports\n", __func__);
		return 0;
	}
}

void __iomem *__a375_ioremap(unsigned long addr, unsigned long size, unsigned int mtype)
{
	if (!is_a375_pcie_phys_memory(addr))
		return __arm_ioremap(addr, size, mtype);

	if ((addr >= PEX0_MEM_PHYS_BASE) && (addr < (PEX0_MEM_PHYS_BASE + PEX0_MEM_SIZE)))
		return (void __iomem *)(PEX0_MEM_VIRT_BASE + (addr - PEX0_MEM_PHYS_BASE));
	else
		return (void __iomem *)(PEX1_MEM_VIRT_BASE + (addr - PEX1_MEM_PHYS_BASE));

}
EXPORT_SYMBOL(__a375_ioremap);

void __a375_iounmap(void __iomem *addr)
{
	if (is_a375_pcie_virt_memory(addr))
		return;
	else
		return __iounmap(addr);
}
EXPORT_SYMBOL(__a375_iounmap);

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
#ifdef MY_ABC_HERE
extern void syno_mv_net_shutdown();
#endif
#define UART1_REG(x)		(PORT1_BASE + ((UART_##x) << 2))
#define SET8N1			0x3
#define SOFTWARE_SHUTDOWN	0x31
#define SOFTWARE_REBOOT		0x43
extern void synology_gpio_init(void);

static void synology_power_off(void)
{
#ifdef MY_ABC_HERE
	/* platform driver will not shutdown when poweroff */
	syno_mv_net_shutdown();
#endif
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_SHUTDOWN, UART1_REG(TX));
}

static void synology_restart(char mode, const char *cmd)
{
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_REBOOT, UART1_REG(TX));

	/* delay for uart1 send the request to uP */
	mdelay(10);
        /* Calls original reset function for models those do not use uP
        * I.e. USB Station. */
        arm_machine_restart(mode, cmd);
}
#endif /* CONFIG_SYNO_ARMADA_ARCH_V2 */

static void __init a375_board_init(void)
{
	mvBoardEnvInit();
	if (mvCtrlEnvInit())
		pr_err("%s: Error: ctrlEnv init failed.\n", __func__);

	a375_init_cpu_mbus();
	a375_init_l2x0_cache();

	/* Init the CPU windows setting and the access protection windows. */
	if (mvCpuIfInit(mv_sys_map())) {
		pr_err("%s: Error: cpu memory windows init failed.\n",
		       __func__);
	}

#ifdef CONFIG_AURORA_IO_CACHE_COHERENCY
	a375_init_iocc();
#endif

	elf_hwcap &= ~HWCAP_JAVA;

	serial_initialize(0);
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
	serial_initialize(1);
#endif

	mvCpuIfAddDecShow();
	print_board_info();

	a375_rtc_init();
	a375_i2c_init();
	a375_init_eth();
	a375_sata_init(&a375_sata_data);
	a375_xor_init();
	a375_spi_init();
	a375_sdio_init();
	a375_nand_nfc_init();

	a375_usb_init();
	a375_hwmon_init();
#if 1
	a375_gpio_init();
#endif

#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	pm_power_off = synology_power_off;
	arm_pm_restart = synology_restart;
	synology_gpio_init();
#endif

}

MACHINE_START(ARMADA_375, "Marvell Armada-375 Board")
	.atag_offset = BOOT_PARAMS_OFFSET,
	.map_io = a375_map_io,
	.init_irq = a375_init_irq,
	.timer = &a375_timer,
	.init_machine = a375_board_init,
MACHINE_END
