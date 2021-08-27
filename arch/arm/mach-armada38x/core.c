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
#include <linux/ethtool.h>
#include <linux/mv643xx_i2c.h>
#include <asm/smp_scu.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <mach/system.h>

#include <linux/etherdevice.h>
#include <linux/tty.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <asm/serial.h>

#include <mach/serial.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/mvUnitMap.h"
#include "cpu/mvCpu.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "mvSysHwConfig.h"

#if defined(CONFIG_SATA_AHCI_MV)
#include "mvSysSataConfig.h"
#include <linux/ahci_platform.h>
#endif

#ifdef CONFIG_MTD_NAND_NFC
#include "mv_mtd/nand_nfc.h"
#endif

#if defined(CONFIG_MV_INCLUDE_SDIO)
#include <linux/platform_data/pxa_sdhci.h>
#include <linux/mmc/sdhci.h>
#include <linux/mmc/host.h>
#endif

#ifdef CONFIG_MV_INCLUDE_XOR
#include <plat/mv_xor.h>
#endif

#if defined(CONFIG_MV_ETH_NETA)
#include <linux/mv_neta.h>
#endif

#if defined(CONFIG_MV_INCLUDE_CESA)
#include "cesa/mvCesa.h"
#endif

#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"
#include "mvSysEthPhyApi.h"

#include <asm/hardware/cache-l2x0.h>
#include <asm/hardware/gic.h>
#include "ca9x2.h"
#include "core.h"

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

struct mbus_dram_target_info a38x_mbus_dram_info;

/*******************************************************************************
 * Early Printk Support
 */
#ifdef MV_INCLUDE_EARLY_PRINTK
#define MV_UART0_LSR    (*(unsigned char *)(INTER_REGS_VIRT_BASE + 0x12000 + 0x14))
#define MV_UART0_THR    (*(unsigned char *)(INTER_REGS_VIRT_BASE + 0x12000 + 0x0))
/*
 * This does not append a newline
 */
void putstr(const char *s)
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
	/* Disabling USB device option for now - U-boot is not ready yet */
	/* mvIsUsbHost = read_tag(tag->u.mv_uboot.isUsbHost); */
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

static void __init a38x_init_cib_mbus_optimizations(void)
{
	void __iomem *addr;
	u32 val;

	addr = (void __iomem *)INTER_REGS_VIRT_BASE;

	/* CIB Read Buffer Select Register - Speed up GbEs through unique allocation to RdBuff 1 */
	__raw_writel(0x88, addr + CIB_READ_BUFFER_SELECT_REG);

	/* MBUS Units Priority Control Register - Prioritize GbEs DRAM access */
	__raw_writel(0xC0C0, addr + MBUS_UNITS_PRIORITY_CONTROL_REG);

	/* MBUS Units Prefetch Control Register - Pre-fetch enable for all IO masters */
	__raw_writel(0xC6F8, addr + MBUS_UNITS_PREFETCH_CONTROL_REG);

	/* CIB Control and Configurations Register - Enable cut-through for reduced latency and disable retry */
	val = __raw_readl(addr + CIB_CTRL_CONFIG_REG);
	val |= (1 << 5) | (1 << 6);
	__raw_writel(val, addr + CIB_CTRL_CONFIG_REG);

	/* SDRAM Interface MBUS Control (Low) Register */
	__raw_writel(0X76543230, addr + SDRAM_INTERFACE_MBUS_CTRL_REG);
}

static void __init a38x_init_cpu_mbus(void)
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
	a38x_mbus_dram_info.mbus_dram_target_id = TARGET_DDR;
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
			w = &a38x_mbus_dram_info.cs[cs++];
			w->cs_index = i;
			w->mbus_attr = 0xf & ~(1 << i);
			w->mbus_attr |= coherency_status << 4;
			w->base = base & 0xff000000;
			w->size = (size | 0x00ffffff) + 1;
		}
	}
	a38x_mbus_dram_info.num_cs = cs;
}

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
static struct mv64xxx_i2c_pdata a38x_i2c_pdata = {
	.freq_m = 4, /* assumes 200 MHz TCLK giving Fscl = 62.5 kHz */
	.freq_n = 5,
	.timeout = 1000, /* Default timeout of 1 second */
};

static struct resource a38x_i2c_0_resources[] = {
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

static struct platform_device a38x_i2c0 = {
	.name = MV64XXX_I2C_CTLR_NAME,
	.id = 0,
	.num_resources = ARRAY_SIZE(a38x_i2c_0_resources),
	.resource = a38x_i2c_0_resources,
	.dev = {
		.platform_data = &a38x_i2c_pdata,
	},
};

static struct resource a38x_i2c_1_resources[] = {
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

static struct platform_device a38x_i2c1 = {
	.name = MV64XXX_I2C_CTLR_NAME,
	.id = 1,
	.num_resources = ARRAY_SIZE(a38x_i2c_1_resources),
	.resource = a38x_i2c_1_resources,
	.dev = {
		.platform_data = &a38x_i2c_pdata,
	},
};
#endif

static void __init a38x_i2c_init(void)
{
#ifdef CONFIG_I2C_MV64XXX
	if (mvUnitMapIsMine(I2C0) == MV_TRUE)
		platform_device_register(&a38x_i2c0);

	if (mvCtrlSocUnitInfoNumGet(I2C_UNIT_ID) >= 1 &&
	    mvUnitMapIsMine(I2C1) == MV_TRUE)
		platform_device_register(&a38x_i2c1);
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
	.id			= 0,
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
static struct resource mv_sdhci_resources[] = {
	[0] = {
	       .start = INTER_REGS_PHYS_BASE + MV_SDMMC_REGS_OFFSET,
	       .end = INTER_REGS_PHYS_BASE + MV_SDMMC_REGS_OFFSET + SZ_1K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INTER_REGS_PHYS_BASE + MV_SDMMC_WINDOWS_REGS_OFFSET,
	       .end = INTER_REGS_PHYS_BASE + MV_SDMMC_WINDOWS_REGS_OFFSET + 0xff,
	       .flags = IORESOURCE_MEM,
	       },
	[2] = {
	       .start = IRQ_GLOBAL_SDIO,
	       .end = IRQ_GLOBAL_SDIO,
	       .flags = IORESOURCE_IRQ,
	       },

};

static u64 mv_sdhci_dmamask = 0xffffffffUL;

static struct sdhci_pxa_platdata mv_sdhci_data = {
	.clk_delay_cycles = 0x1f,
	.quirks = SDHCI_QUIRK_INVERTED_WRITE_PROTECT
		| SDHCI_QUIRK_BROKEN_CARD_DETECTION,
	.host_caps = MMC_CAP_8_BIT_DATA,
	.dram = &a38x_mbus_dram_info,
};

static struct platform_device mv_sdhci_plat = {
	.name = "sdhci-pxav3",
	.id = -1,
	.dev = {
		.dma_mask = &mv_sdhci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data = &mv_sdhci_data,
		},
	.num_resources = ARRAY_SIZE(mv_sdhci_resources),
	.resource = mv_sdhci_resources,
};
#endif

void __init a38x_sdhci_init(void)
{
#ifdef CONFIG_MV_INCLUDE_SDIO
	if (mvUnitMapIsMine(SDIO) != MV_TRUE)
		return;
	platform_device_register(&mv_sdhci_plat);
#endif
}

/*******************************************************************************
 * USB
 */

void __init a38x_usb_init(void)
{
#ifdef CONFIG_MV_INCLUDE_USB
	mv_usb_init(&a38x_mbus_dram_info);
#endif
}

/*******************************************************************************
 * GBE
 */
#ifdef CONFIG_MV_ETHERNET
#if defined(CONFIG_MV_ETH_LEGACY)
static struct platform_device mv88fx_eth = {
	.name = "mv88fx_eth",
	.id = 0,
	.num_resources = 0,
};
#endif /* CONFIG_MV_ETH_LEGACY */

#if defined(CONFIG_MV_ETH_NETA) || defined(CONFIG_MV_ETH_NETA_MODULE)
static void mv_neta_giga_pdev_register(struct platform_device *pdev)
{
	struct mv_neta_pdata *plat_data = (struct mv_neta_pdata *)pdev->dev.platform_data;
	int speed, port = pdev->id;

	/* Global Parameters */
	plat_data->ctrl_model = mvCtrlModelGet();
	plat_data->ctrl_rev = mvCtrlRevGet();
	plat_data->pclk = mvCpuPclkGet();
	plat_data->tclk = mvBoardTclkGet();
	plat_data->max_port = mvCtrlEthMaxPortGet();
	plat_data->max_cpu = mvCtrlEthMaxCPUsGet();
	/* Per port parameters */
	plat_data->cpu_mask  = (1 << nr_cpu_ids) - 1;
	plat_data->phy_addr = mvBoardPhyAddrGet(port);
	plat_data->is_sgmii = mvBoardIsPortInSgmii(port);
	plat_data->is_rgmii = mvBoardIsPortInRgmii(port);
	plat_data->duplex = DUPLEX_FULL;

	if (port < MV_UBOOT_ETH_PORTS) {
		plat_data->mtu = mvMtu[port];
		if (plat_data->mtu == 0)
			plat_data->mtu = 1500;
		memcpy(plat_data->mac_addr, mvMacAddr[port], 6);
		if (is_zero_ether_addr(plat_data->mac_addr))
			pr_warning("Warning: port #%d - zero MAC address\n", port);
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
	pr_info("Register platform device: %s_%d\n", pdev->name, pdev->id);
	platform_device_register(pdev);
}
static struct resource mv_neta_ge0_resources[] = {
	{
		.start          = IRQ_PRIV_PORT0_TH_RXTX,
		.end            = IRQ_PRIV_PORT0_TH_RXTX,
		.flags          = IORESOURCE_IRQ,
	},
};
static struct mv_neta_pdata mv_neta_ge0_pdata = {
	.mtu = 1500,
	.phy_addr = 0,
	.tx_csum_limit = MV_ETH_TX_CSUM_MAX_SIZE,
};
static struct platform_device mv_neta_ge0_plat = {
	.name           = MV_NETA_PORT_NAME,
	.id		= 0,
	.num_resources  = ARRAY_SIZE(mv_neta_ge0_resources),
	.resource       = mv_neta_ge0_resources,
	.dev            = {
		.platform_data = &mv_neta_ge0_pdata,
	},
};
static struct resource mv_neta_ge1_resources[] = {
	{
		.start          = IRQ_PRIV_PORT1_TH_RXTX,
		.end            = IRQ_PRIV_PORT1_TH_RXTX,
		.flags          = IORESOURCE_IRQ,
	},
};
static struct mv_neta_pdata mv_neta_ge1_pdata = {
	.mtu = 1500,
	.phy_addr = -1,
	.tx_csum_limit = MV_ETH_TX_CSUM_MAX_SIZE_SMALL,
};
static struct platform_device mv_neta_ge1_plat = {
	.name           = MV_NETA_PORT_NAME,
	.id             = 1,
	.num_resources  = ARRAY_SIZE(mv_neta_ge1_resources),
	.resource       = mv_neta_ge1_resources,
	.dev            = {
		.platform_data = &mv_neta_ge1_pdata,
	},
};
static struct resource mv_neta_ge2_resources[] = {
	{
		.start          = IRQ_PRIV_PORT2_TH_RXTX,
		.end            = IRQ_PRIV_PORT2_TH_RXTX,
		.flags          = IORESOURCE_IRQ,
	},
};
static struct mv_neta_pdata mv_neta_ge2_pdata = {
	.mtu = 1500,
	.phy_addr = -1,
	.tx_csum_limit = MV_ETH_TX_CSUM_MAX_SIZE_SMALL,
};
static struct platform_device mv_neta_ge2_plat = {
	.name           = MV_NETA_PORT_NAME,
	.id             = 2,
	.num_resources  = ARRAY_SIZE(mv_neta_ge2_resources),
	.resource       = mv_neta_ge2_resources,
	.dev            = {
		.platform_data = &mv_neta_ge2_pdata,
	},
};
#endif /* CONFIG_MV_ETH_NETA || CONFIG_MV_ETH_NETA_MODULE */

static void __init eth_init(void)
{
#if defined(CONFIG_MV_ETH_LEGACY)
	platform_device_register(&mv88fx_eth);
#endif /* CONFIG_MV_ETH_LEGACY */

#if defined(CONFIG_MV_ETH_NETA) || defined(CONFIG_MV_ETH_NETA_MODULE)
	MV_U32 devId;

	devId = mvCtrlModelGet();

	if ((mvUnitMapIsMine(ETH0) == MV_TRUE) && (mvBoardIsEthConnected(0)) &&
	    (mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, 0))) {
		mv_neta_giga_pdev_register(&mv_neta_ge0_plat);
	}
	if ((mvUnitMapIsMine(ETH1) == MV_TRUE) && (mvBoardIsEthConnected(1)) &&
	    (mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, 1))) {
		mv_neta_giga_pdev_register(&mv_neta_ge1_plat);
	}

#endif /* CONFIG_MV_ETH_NETA) || CONFIG_MV_ETH_NETA_MODULE */
}
#endif /* CONFIG_MV_ETHERNET */

void __init a38x_init_eth(void)
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

static void __init a38x_gpio_init(void)
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

static void __init a38x_rtc_init(void)
{
	platform_device_register_simple("rtc-mv", -1, rtc_resource, 2);
}

/*******************************************************************************
 * SATA
 */
#ifdef CONFIG_SATA_AHCI_MV
#define SATA_UNIT0_PHYS_BASE (INTER_REGS_PHYS_BASE | MV_SATA_UNIT_REGS_BASE(0))
#define SATA_UNIT1_PHYS_BASE (INTER_REGS_PHYS_BASE | MV_SATA_UNIT_REGS_BASE(1))

static u64 a38x_sata_dmamask = DMA_BIT_MASK(32);

static struct mv_sata_platform_data a38x_sata_pdata = {
	.dram = &a38x_mbus_dram_info,
};

static struct resource a38x_sata_unit0_resources[] = {
	{
	 .name = "sata unit0 base",
	 .start = SATA_UNIT0_PHYS_BASE,
	 .end = SATA_UNIT0_PHYS_BASE + 0x1fff,
	 .flags = IORESOURCE_MEM,
	 }, {
	     .name = "sata unit0 irq",
	     .start = IRQ_GLOBAL_SATA_UNIT0,
	     .end = IRQ_GLOBAL_SATA_UNIT0,
	     .flags = IORESOURCE_IRQ,
	     },
};

static struct platform_device a38x_sata_unit0 = {
	.name = "ahci_mv",
	.id = 0,
	.dev = {
		.platform_data = &a38x_sata_pdata,
		.dma_mask = &a38x_sata_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		},
	.num_resources = ARRAY_SIZE(a38x_sata_unit0_resources),
	.resource = a38x_sata_unit0_resources,
};

static struct resource a38x_sata_unit1_resources[] = {
	{
	 .name = "sata unit1 base",
	 .start = SATA_UNIT1_PHYS_BASE,
	 .end = SATA_UNIT1_PHYS_BASE + 0x1fff,
	 .flags = IORESOURCE_MEM,
	 }, {
	     .name = "sata unit1 irq",
	     .start = IRQ_GLOBAL_SATA_UNIT1,
	     .end = IRQ_GLOBAL_SATA_UNIT1,
	     .flags = IORESOURCE_IRQ,
	     },
};

static struct platform_device a38x_sata_unit1 = {
	.name = "ahci_mv",
	.id = 1,
	.dev = {
		.platform_data = &a38x_sata_pdata,
		.dma_mask = &a38x_sata_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		},
	.num_resources = ARRAY_SIZE(a38x_sata_unit1_resources),
	.resource = a38x_sata_unit1_resources,
};
#endif /* CONFIG_SATA_AHCI_MV */

static void __init a38x_sata_init(void)
{
#ifdef CONFIG_SATA_AHCI_MV
	if (mvUnitMapIsMine(SATA) != MV_TRUE)
		return;

	platform_device_register(&a38x_sata_unit0);
	platform_device_register(&a38x_sata_unit1);
#endif /* CONFIG_SATA_AHCI_MV */
}

/*******************************************************************************
 * SoC hwmon Thermal Sensor
 */
static void __init a38x_hwmon_init(void)
{
	if (mvUnitMapIsMine(HWMON) == MV_TRUE)
		platform_device_register_simple("a38x-temp", 0, NULL, 0);
}

/*******************************************************************************
 * NAND controller
 */
#ifdef CONFIG_MTD_NAND_NFC
static struct resource a38x_nfc_resources[] = {
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

static struct nfc_platform_data a38x_nfc_data = {
	.nfc_width = 8,
	.num_devs = 1,
	.num_cs = 1,
	.use_dma = 0,
	.ecc_type = MV_NFC_ECC_BCH_2K,
	.parts = nand_parts_info,
	.nr_parts = ARRAY_SIZE(nand_parts_info),
};

static struct platform_device a38x_nfc = {
	.name = "armada-nand",
	.id = 0,
	.dev = {
		.platform_data = &a38x_nfc_data,
		},
	.num_resources = ARRAY_SIZE(a38x_nfc_resources),
	.resource = a38x_nfc_resources,
};
#endif

static void __init a38x_nand_nfc_init(void)
{
#ifdef CONFIG_MTD_NAND_NFC
	if (mvUnitMapIsMine(NAND) != MV_TRUE)
		return;

	/* Check for ganaged mode */
	if (nfcConfig) {
		if (strncmp(nfcConfig, "ganged", 6) == 0) {
			a38x_nfc_data.nfc_width = 16;
			a38x_nfc_data.num_devs = 2;
			nfcConfig += 7;
		}

		/* Check for ECC type directive */
		if (strcmp(nfcConfig, "8bitecc") == 0)
			a38x_nfc_data.ecc_type = MV_NFC_ECC_BCH_1K;
		else if (strcmp(nfcConfig, "12bitecc") == 0)
			a38x_nfc_data.ecc_type = MV_NFC_ECC_BCH_704B;
		else if (strcmp(nfcConfig, "16bitecc") == 0)
			a38x_nfc_data.ecc_type = MV_NFC_ECC_BCH_512B;
	}

	a38x_nfc_data.tclk = mvBoardTclkGet();

	platform_device_register(&a38x_nfc);
#endif
}

/*******************************************************************************
 * XOR
 */
#ifdef CONFIG_MV_INCLUDE_XOR
static struct mv_xor_platform_shared_data a38x_xor_shared_data = {
	.dram = &a38x_mbus_dram_info,
};

static u64 a38x_xor_dmamask = DMA_BIT_MASK(32);

/*
 * XOR0
 */
static struct resource a38x_xor0_shared_resources[] = {
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

static struct platform_device a38x_xor0_shared = {
	.name = MV_XOR_SHARED_NAME,
	.id = 0,
	.dev = {
		.platform_data = &a38x_xor_shared_data,
		},
	.num_resources = ARRAY_SIZE(a38x_xor0_shared_resources),
	.resource = a38x_xor0_shared_resources,
};

static struct resource a38x_xor00_resources[] = {
	[0] = {
	       .start = IRQ_GLOBAL_XOR0_CHAN0,
	       .end = IRQ_GLOBAL_XOR0_CHAN0,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mv_xor_platform_data a38x_xor00_data = {
	.shared = &a38x_xor0_shared,
	.hw_id = 0,
	.pool_size = PAGE_SIZE,
};

static struct platform_device a38x_xor00_channel = {
	.name = MV_XOR_NAME,
	.id = 0,
	.num_resources = ARRAY_SIZE(a38x_xor00_resources),
	.resource = a38x_xor00_resources,
	.dev = {
		.dma_mask = &a38x_xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a38x_xor00_data,
		},
};

static struct resource a38x_xor01_resources[] = {
	[0] = {
	       .start = IRQ_GLOBAL_XOR0_CHAN1,
	       .end = IRQ_GLOBAL_XOR0_CHAN1,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mv_xor_platform_data a38x_xor01_data = {
	.shared = &a38x_xor0_shared,
	.hw_id = 1,
	.pool_size = PAGE_SIZE,
};

static struct platform_device a38x_xor01_channel = {
	.name = MV_XOR_NAME,
	.id = 1,
	.num_resources = ARRAY_SIZE(a38x_xor01_resources),
	.resource = a38x_xor01_resources,
	.dev = {
		.dma_mask = &a38x_xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a38x_xor01_data,
		},
};

static void __init a38x_xor0_init(void)
{
	if (mvUnitMapIsMine(XOR0) != MV_TRUE)
		return;

	platform_device_register(&a38x_xor0_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	dma_cap_set(DMA_MEMCPY, a38x_xor00_data.cap_mask);
	dma_cap_set(DMA_XOR, a38x_xor00_data.cap_mask);
	platform_device_register(&a38x_xor00_channel);

	dma_cap_set(DMA_MEMCPY, a38x_xor01_data.cap_mask);
	dma_cap_set(DMA_MEMSET, a38x_xor01_data.cap_mask);
	dma_cap_set(DMA_XOR, a38x_xor01_data.cap_mask);
	platform_device_register(&a38x_xor01_channel);
}

/*
 * XOR1
 */
static struct resource a38x_xor1_shared_resources[] = {
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

static struct platform_device a38x_xor1_shared = {
	.name = MV_XOR_SHARED_NAME,
	.id = 1,
	.dev = {
		.platform_data = &a38x_xor_shared_data,
		},
	.num_resources = ARRAY_SIZE(a38x_xor1_shared_resources),
	.resource = a38x_xor1_shared_resources,
};

static struct resource a38x_xor10_resources[] = {
	[0] = {
	       .start = IRQ_GLOBAL_XOR1_CHAN0,
	       .end = IRQ_GLOBAL_XOR1_CHAN0,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mv_xor_platform_data a38x_xor10_data = {
	.shared = &a38x_xor1_shared,
	.hw_id = 0,
	.pool_size = PAGE_SIZE,
};

static struct platform_device a38x_xor10_channel = {
	.name = MV_XOR_NAME,
	.id = 2,
	.num_resources = ARRAY_SIZE(a38x_xor10_resources),
	.resource = a38x_xor10_resources,
	.dev = {
		.dma_mask = &a38x_xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a38x_xor10_data,
		},
};

static struct resource a38x_xor11_resources[] = {
	[0] = {
	       .start = IRQ_GLOBAL_XOR1_CHAN1,
	       .end = IRQ_GLOBAL_XOR1_CHAN1,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct mv_xor_platform_data a38x_xor11_data = {
	.shared = &a38x_xor1_shared,
	.hw_id = 1,
	.pool_size = PAGE_SIZE,
};

static struct platform_device a38x_xor11_channel = {
	.name = MV_XOR_NAME,
	.id = 3,
	.num_resources = ARRAY_SIZE(a38x_xor11_resources),
	.resource = a38x_xor11_resources,
	.dev = {
		.dma_mask = &a38x_xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &a38x_xor11_data,
		},
};

static void __init a38x_xor1_init(void)
{
	if (mvUnitMapIsMine(XOR1) != MV_TRUE)
		return;

	platform_device_register(&a38x_xor1_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	dma_cap_set(DMA_MEMCPY, a38x_xor10_data.cap_mask);
	dma_cap_set(DMA_XOR, a38x_xor10_data.cap_mask);
	platform_device_register(&a38x_xor10_channel);

	dma_cap_set(DMA_MEMCPY, a38x_xor11_data.cap_mask);
	dma_cap_set(DMA_MEMSET, a38x_xor11_data.cap_mask);
	dma_cap_set(DMA_XOR, a38x_xor11_data.cap_mask);
	platform_device_register(&a38x_xor11_channel);

}
#endif

static void __init a38x_xor_init(void)
{
#ifdef CONFIG_MV_INCLUDE_XOR
	a38x_xor0_init();
	a38x_xor1_init();
#endif
}

/*******************************************************************************
 * SPI
 */
static void a38x_spi_init(void)
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

	pr_info("\n  Marvell Armada-38x");

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
#endif
#ifdef CONFIG_MV_AMP_ENABLE
	mvUnitMapPrint();
#endif
	pr_info("\n");
}

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
static void __init a38x_init_iocc(void)
{

#if !defined(CONFIG_SMP)
	void __iomem *scu_base = (void __iomem *)(INTER_REGS_VIRT_BASE + A9_MPCORE_SCU);
	scu_enable(scu_base);
#endif

	return;
}
#endif /* CONFIG_AURORA_IO_CACHE_COHERENCY */

static void __init a38x_init_l2x0_cache(void)
{
#ifdef CONFIG_CACHE_L2X0
	void __iomem *l2x0_base =
	    (void __iomem *)(INTER_REGS_VIRT_BASE + MV_CA9X2_L2CC_OFFSET);

	/* L2X0 Power Control */
	__raw_writel(L2X0_DYNAMIC_CLK_GATING_EN, l2x0_base + L2X0_POWER_CTRL);

	/* 88F6820 - 16 ways, 88F6810 - 8 ways */
	if (mvCtrlGetCpuNum())
		l2x0_init(l2x0_base, 0x00470000, 0xfe0fffff);
	else
		l2x0_init(l2x0_base, 0x00460000, 0xfe0fffff);
#endif
}

static inline int is_a38x_pcie_phys_memory(u32 addr)
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

static inline int is_a38x_pcie_virt_memory(u32 addr)
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

void __iomem *__a38x_ioremap(unsigned long addr, unsigned long size, unsigned int mtype)
{
	if (!is_a38x_pcie_phys_memory(addr))
		return __arm_ioremap(addr, size, mtype);

	if ((addr >= PEX0_MEM_PHYS_BASE) && (addr < (PEX0_MEM_PHYS_BASE + PEX0_MEM_SIZE)))
		return (void __iomem *)(PEX0_MEM_VIRT_BASE + (addr - PEX0_MEM_PHYS_BASE));
	else
		return (void __iomem *)(PEX1_MEM_VIRT_BASE + (addr - PEX1_MEM_PHYS_BASE));

}
EXPORT_SYMBOL(__a38x_ioremap);

void __a38x_iounmap(void __iomem *addr)
{
	if (is_a38x_pcie_virt_memory(addr))
		return;
	else
		return __iounmap(addr);
}
EXPORT_SYMBOL(__a38x_iounmap);

static void __init a38x_board_init(void)
{
	mvBoardEnvInit();
	if (mvCtrlEnvInit())
		pr_err("%s: Error: ctrlEnv init failed.\n", __func__);

	a38x_init_cib_mbus_optimizations();
	a38x_init_cpu_mbus();
	a38x_init_l2x0_cache();

	/* Init the CPU windows setting and the access protection windows. */
	if (mvCpuIfInit(mv_sys_map())) {
		pr_err("%s: Error: cpu memory windows init failed.\n",
		       __func__);
	}

#ifdef CONFIG_AURORA_IO_CACHE_COHERENCY
	a38x_init_iocc();
#endif

	elf_hwcap &= ~HWCAP_JAVA;

	serial_initialize(0);

	mvCpuIfAddDecShow();
	print_board_info();

	a38x_init_eth();
	a38x_xor_init();
	a38x_usb_init();
	a38x_sata_init();
	a38x_sdhci_init();
	a38x_nand_nfc_init();
	a38x_spi_init();
	a38x_i2c_init();

#if 0
	a38x_rtc_init();
	a38x_gpio_init();
	a38x_hwmon_init();
#endif
}

MACHINE_START(ARMADA_38X, "Marvell Armada-38x Board")
	.atag_offset = BOOT_PARAMS_OFFSET,
	.map_io = a38x_map_io,
	.init_irq = a38x_init_irq,
	.timer = &a38x_timer,
	.init_machine = a38x_board_init,
MACHINE_END
