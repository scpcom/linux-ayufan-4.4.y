#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/mbus.h>
#include <linux/memblock.h>
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
#include <linux/mv_neta.h>
#include <asm/serial.h>
#include <plat/cache-aurora-l2.h>

#include <mach/serial.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvUnitMap.h"
#include "ctrlEnv/mvSemaphore.h"
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

#include <plat/mv_xor.h>

#include <linux/i2c.h>
#include <linux/mv643xx_i2c.h>
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"

#include "mvSysSpiApi.h"

#include "mvSysEthPhyApi.h"
#include "eth-phy/mvEthPhy.h"

#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include <mach/dove_bl.h>

#ifdef CONFIG_MTD_NAND_NFC
#include "mv_mtd/nand_nfc.h"
#endif

#ifdef CONFIG_MV_INCLUDE_USB
#include "usb/mvUsb.h"
#endif

#define MV_COHERENCY_FABRIC_CTRL_REG		(MV_COHERENCY_FABRIC_OFFSET + 0x0)
#define MV_COHERENCY_FABRIC_CFG_REG		(MV_COHERENCY_FABRIC_OFFSET + 0x4)

#ifdef CONFIG_FB_DOVE
extern unsigned int lcd0_enable;
extern int lcd_panel;
#endif
extern unsigned int irq_int_type[];
extern void __init axp_map_io(void);
extern void __init mv_init_irq(void);
extern struct sys_timer axp_timer;
extern void axp_timer_resume(void);
extern MV_CPU_DEC_WIN* mv_sys_map(void);
#if defined(CONFIG_MV_INCLUDE_CESA)
extern u32 mv_crypto_virt_base_get(u8 chan);
#endif
extern void axp_init_irq(void);
extern void __init set_core_count(unsigned int cpu_count);
extern unsigned int group_cpu_mask;

static char arr[256];
MV_U32 mvTclk = 166666667;
MV_U32 mvSysclk = 200000000;

#ifdef CONFIG_MV_INCLUDE_GIG_ETH
MV_U8 mvMacAddr[CONFIG_MV_ETH_PORTS_NUM][6];
MV_U16 mvMtu[CONFIG_MV_ETH_PORTS_NUM] = {0};
#endif

#define DDR_BASE_CS_OFF(n)	(0x0180 + ((n) << 3))
#define DDR_SIZE_CS_OFF(n)	(0x0184 + ((n) << 3))
#define TARGET_DDR		0
#define COHERENCY_STATUS_SHARED_NO_L2_ALLOC	0x1

struct mbus_dram_target_info armadaxp_mbus_dram_info;

#ifdef CONFIG_ARMADA_XP_REV_Z1
  
#undef XOR0_ENABLE
#else
  
#define XOR0_ENABLE
#endif

#ifdef MV_INCLUDE_EARLY_PRINTK
#define MV_UART0_LSR 	(*(volatile unsigned char *)(INTER_REGS_BASE + 0x12000 + 0x14))
#define MV_UART0_THR	(*(volatile unsigned char *)(INTER_REGS_BASE + 0x12000 + 0x0 ))
#define MV_UART1_LSR    (*(volatile unsigned char *)(INTER_REGS_BASE + 0x12100 + 0x14))
#define MV_UART1_THR    (*(volatile unsigned char *)(INTER_REGS_BASE + 0x12100 + 0x0 ))
#define MV_SERIAL_BASE 	((unsigned char *)(INTER_REGS_BASE + 0x12000 + 0x0 ))
#define DEV_REG		(*(volatile unsigned int *)(INTER_REGS_BASE + 0x40000))
#define CLK_REG         (*(volatile unsigned int *)(INTER_REGS_BASE + 0x2011c))
 
static void putstr(const char *s)
{
	unsigned int model;

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

#ifdef CONFIG_BE8_ON_LE
#define read_tag(a)    le32_to_cpu(a)
#define read_mtu(a)    le16_to_cpu(a)
#else
#define read_tag(a)    a
#define read_mtu(a)    a
#endif

extern MV_U32 gBoardId; 
extern unsigned int elf_hwcap;
extern u32 mvIsUsbHost;

static int __init parse_tag_mv_uboot(const struct tag *tag)
{
    	unsigned int mvUbootVer = 0;
	int i = 0;

	printk("Using UBoot passing parameters structure\n");
	mvUbootVer = read_tag(tag->u.mv_uboot.uboot_version);
#ifdef CONFIG_MV_INCLUDE_USB
	mvIsUsbHost = read_tag(tag->u.mv_uboot.isUsbHost);
#endif
	gBoardId =  (mvUbootVer & 0xff);

#ifdef CONFIG_MV_INCLUDE_GIG_ETH
	for (i = 0; i < CONFIG_MV_ETH_PORTS_NUM; i++) {
#if defined (CONFIG_OVERRIDE_ETH_CMDLINE)
		memset(mvMacAddr[i], 0, 6);
		mvMtu[i] = 0;
#else
		memcpy(mvMacAddr[i], tag->u.mv_uboot.macAddr[i], 6);
		mvMtu[i] = read_mtu(tag->u.mv_uboot.mtu[i]);
#endif
	}
#endif

#ifdef CONFIG_MV_NAND
                
	if ((mvUbootVer >> 8) >= 0x3040c)
		mv_nand_ecc = read_tag(tag->u.mv_uboot.nand_ecc);
	else
		mv_nand_ecc = 1;  
#endif
	return 0;
}

__tagtable(ATAG_MV_UBOOT, parse_tag_mv_uboot);

#ifdef CONFIG_MV_INCLUDE_USB
#include "mvSysUsbApi.h"
 
static char *usb0Mode = "host";
static char *usb1Mode = "host";
static char *usb2Mode = "device";
int mv_usb0_cmdline_config(char *s);
int mv_usb1_cmdline_config(char *s);
int mv_usb2_cmdline_config(char *s);
__setup("usb0Mode=", mv_usb0_cmdline_config);
__setup("usb1Mode=", mv_usb1_cmdline_config);
__setup("usb2Mode=", mv_usb2_cmdline_config);

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

int mv_usb2_cmdline_config(char *s)
{
    usb2Mode = s;
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
unsigned int l0_disable_flag = 0;		 
static int __init l0_disable_setup(char *__unused)
{
     l0_disable_flag = 1;
     return 1;
}

__setup("l0_disable", l0_disable_setup);
#endif

#ifndef CONFIG_SHEEVA_ERRATA_ARM_CPU_5315
unsigned int sp_enable_flag = 0;		 
static int __init spec_prefesth_setup(char *__unused)
{
     sp_enable_flag = 1;
     return 1;
}

__setup("sp_enable", spec_prefesth_setup);
#endif

#ifdef CONFIG_JTAG_DEBUG
	MV_U32 support_wait_for_interrupt = 0x0;
#else
	MV_U32 support_wait_for_interrupt = 0x1;
#endif

static int __init noWFI_setup(char *__unused)
{
     support_wait_for_interrupt = 0;
     return 1;
}

__setup("noWFI", noWFI_setup);

char *nfcConfig = NULL;
static int __init nfcConfig_setup(char *s)
{
	nfcConfig = s;
	return 1;
}
__setup("nfcConfig=", nfcConfig_setup);

#ifdef CONFIG_SMP
unsigned long mv_cpu_count = NR_CPUS;
static int __init mv_cpu_count_setup(char *s)
{
	int fail;
	unsigned long cpu_count;

	while (isspace(*s))
		s++;

	fail = strict_strtoul(s, 10, &cpu_count);
	if(fail == 0)
	{
		set_core_count(cpu_count);
	}

	return 1;
}
__setup("mv_cpu_count=", mv_cpu_count_setup);

static int __init mv_rsrc_setup(char *s)
{
	char* rsrc = strchr(s, ' ');

	if (rsrc) (*rsrc) = '\0';
	 
	if (MV_FALSE == mvUnitMapSetup(s, strstr))
		printk(KERN_ERR "Invalid resource string %s\n", s);

	mvUnitMapSetRsrcLimited(MV_TRUE);

	return 1;
}
__setup("mv_rsrc=", mv_rsrc_setup);
#endif  

#ifdef CONFIG_MV_AMP_ENABLE
unsigned int sh_mem_base = 0, sh_mem_size = 0;
static int __init mv_shared_mem_setup(char *s)
{
	char *delim = strchr(s, ':');
	char *base_str  = s;
	char *size_str  = delim + 1;
	int fail;
	void *sh_virt_base;

	if(delim == NULL){
		printk(KERN_WARNING "AMP: No delimiter in shared memory string %s. use format mv_sh_mem=base:size\n", s);
		return 1;
	}

	*delim = '\0';

	fail  = strict_strtoul(base_str, 16, &sh_mem_base);
	fail |= strict_strtoul(size_str, 16, &sh_mem_size);

	if(fail)
		printk(KERN_WARNING "AMP: Bad shared memory string %s:%s. Cant extract valid values\n", base_str,size_str);

	return 1;
}
__setup("mv_sh_mem=", mv_shared_mem_setup);
#endif  

#ifdef CONFIG_MV_IPC_DRIVER
int ipc_target_cpu;
static int __init mv_ipc_setup(char *s)
{
	int fail;
	unsigned int cpu_count;

	while (isspace(*s))
		s++;

	fail = strict_strtoul(s, 10, &ipc_target_cpu);
	if(fail) {
		printk(KERN_WARNING "IPC: Received bad target cpu id %s\n", s);
		ipc_target_cpu = -1;
	}

	return 1;
}
__setup("mv_ipc=", mv_ipc_setup);
#endif

void __init armadaxp_setup_cpu_mbus(void)
{
	int i;
	int cs;
	u8	coherency_status = 0;
	static MV_UNIT_WIN_INFO addrWinMap[MAX_TARGETS + 1];
#if defined(CONFIG_AURORA_IO_CACHE_COHERENCY)
	coherency_status = COHERENCY_STATUS_SHARED_NO_L2_ALLOC;
#endif

	armadaxp_mbus_dram_info.mbus_dram_target_id = TARGET_DDR;
	mvCtrlAddrWinMapBuild(addrWinMap, MAX_TARGETS + 1);
	for (i = 0, cs = 0; i < MAX_TARGETS; i++) {
		struct mbus_dram_window *w;
		if (!MV_TARGET_IS_DRAM(i))
			continue;

		if (addrWinMap[i].enable == MV_FALSE)
			continue;

		if (addrWinMap[i].addrWin.baseHigh)
			 
				continue;

			w = &armadaxp_mbus_dram_info.cs[cs++];
		w->cs_index = (addrWinMap[i].targetId - SDRAM_CS0);
		w->mbus_attr = addrWinMap[i].attrib;
		w->base = addrWinMap[i].addrWin.baseLow;
		w->size = addrWinMap[i].addrWin.size;
	}

	armadaxp_mbus_dram_info.num_cs = cs;
}

static struct mv64xxx_i2c_pdata axp_i2c_pdata = {
       .freq_m         = 8,  
       .freq_n         = 3,
       .timeout        = 1000,  
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

#ifdef CONFIG_FB_DOVE
static struct resource axp_i2c_1_resources[] = {
	{
		.name   = "i2c base",
		.start  = INTER_REGS_PHYS_BASE + MV_TWSI_SLAVE_REGS_OFFSET(1),
		.end    = INTER_REGS_PHYS_BASE + MV_TWSI_SLAVE_REGS_OFFSET(1) + 0x20 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name   = "i2c irq",
		.start  = IRQ_AURORA_I2C1,
		.end    = IRQ_AURORA_I2C1,
		.flags  = IORESOURCE_IRQ,
	},
};
#endif

static struct platform_device axp_i2c0 = {
	.name           = MV64XXX_I2C_CTLR_NAME,
	.id             = 0,
	.num_resources  = ARRAY_SIZE(axp_i2c_0_resources),
	.resource       = axp_i2c_0_resources,
	.dev            = {
		.platform_data = &axp_i2c_pdata,
	},
};

#ifdef CONFIG_FB_DOVE
static struct platform_device axp_i2c1 = {
	.name           = MV64XXX_I2C_CTLR_NAME,
	.id             = 1,
	.num_resources  = ARRAY_SIZE(axp_i2c_1_resources),
	.resource       = axp_i2c_1_resources,
	.dev            = {
		.platform_data = &axp_i2c_pdata,
	},
};
#endif

void __init armadaxp_i2c0_init(void)
{
	if (mvUnitMapIsMine(I2C0) == MV_TRUE)
		platform_device_register(&axp_i2c0);
}

#ifdef CONFIG_FB_DOVE
void __init armadaxp_i2c1_init(void)
{
	if (mvUnitMapIsMine(I2C1) == MV_TRUE)
		platform_device_register(&axp_i2c1);
}
#endif

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
#ifdef CONFIG_SYNO_ARMADA_ARCH
	.id			= 1,
#else
	.id			= 0,
#endif
	.dev			= {
		.platform_data	= aurora_uart1_data,
	},
	.resource		= aurora_uart1_resources,
	.num_resources		= ARRAY_SIZE(aurora_uart1_resources),
};

void __init serial_initialize(int port)
{
	if(port == 0)
	{
		if(mvUnitMapIsMine(UART0) == MV_FALSE){
			printk(KERN_WARNING "uart%d resource not allocated but CONFIG_MV_UART_PORT = %d\n", port, port);
			mvUnitMapSetMine(UART0);
		}

		aurora_uart0_data[0].uartclk = mvBoardTclkGet();
		platform_device_register(&aurora_uart0);
	}
	else
	{
		if(mvUnitMapIsMine(UART1) == MV_FALSE){
			printk(KERN_WARNING "uart%d resource not allocated but CONFIG_MV_UART_PORT = %d\n", port, port);
			mvUnitMapSetMine(UART1);
		}

		aurora_uart1_data[0].uartclk = mvBoardTclkGet();
		platform_device_register(&aurora_uart1);
	}
}

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

void __init armadaxp_sdio_init(void)
{
	if (mvUnitMapIsMine(SDIO) != MV_TRUE)
		return;

	if (MV_TRUE == mvCtrlPwrClckGet(SDIO_UNIT_ID, 0)) {
		int irq_detect = mvBoardSDIOGpioPinGet(BOARD_GPP_SDIO_DETECT);
		static MV_UNIT_WIN_INFO addrWinMap[MAX_TARGETS + 1];

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
}
#endif  

#ifdef CONFIG_MV_ETHERNET
#if defined(CONFIG_MV_ETH_LEGACY)
static struct platform_device mv88fx_eth = {
	.name		= "mv88fx_eth",
	.id		= 0,
	.num_resources	= 0,
};
#elif defined(CONFIG_MV_ETH_NETA)
static struct platform_device mv88fx_neta = {
	.name		= "mv88fx_neta",
	.id		= 0,
	.num_resources	= 0,
};
#else
#error "Ethernet Mode is not defined (should be Legacy or NETA)"
#endif  

static void __init eth_init(void)
{
 struct netaSmpGroupStruct *netaSmpGroupStruct;
        int cpu, found = 0, portMask;

        netaSmpGroupStruct = kzalloc(sizeof(struct netaSmpGroupStruct), GFP_KERNEL);

        if (!netaSmpGroupStruct) {
                        printk(KERN_ERR "no memory for private data\n");
                        return;
        }
        else {

#ifdef  CONFIG_SMP
                netaSmpGroupStruct->cpuMask  = group_cpu_mask;
#else
                netaSmpGroupStruct->cpuMask  = 1;
#endif

                portMask  = (mvUnitMapIsMine(ETH0) == MV_TRUE);
                portMask |= (mvUnitMapIsMine(ETH1) == MV_TRUE) << 1;
                portMask |= (mvUnitMapIsMine(ETH2) == MV_TRUE) << 2;
                portMask |= (mvUnitMapIsMine(ETH3) == MV_TRUE) << 3;

                netaSmpGroupStruct->portMask = portMask;
        }

        for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
                if (MV_BIT_CHECK(netaSmpGroupStruct->cpuMask, cpu))
                        found = 1;
        }
        if (!found) {
                printk(KERN_ERR "%s: cpuMask does not contain any of the CPUs \n", __func__);
                printk(KERN_ERR "%s: not initializing network driver\n", __func__);
                return;
        }
        mv88fx_neta.dev.platform_data = netaSmpGroupStruct;

#if defined(CONFIG_MV_ETH_LEGACY)
        platform_device_register(&mv88fx_eth);
#elif defined(CONFIG_MV_ETH_NETA)
        platform_device_register(&mv88fx_neta);
#endif  
}

#endif  

static struct platform_device mv_gpio = {
	.name	= "mv_gpio",
	.id		= 0,
	.num_resources	= 0,
};

static void __init mv_gpio_init(void)
{
	platform_device_register(&mv_gpio);
}

#ifdef CONFIG_MV_IPC_NET
static struct platform_device mv_ipc_net = {
	.name	= "mv_ipc_net",
	.id		= 0,
	.num_resources	= 0,
	.dev    = {
		.platform_data = (void*)&ipc_target_cpu
	}
};
#endif

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
	if (mvUnitMapIsMine(RTC) == MV_TRUE)
		platform_device_register_simple("rtc-mv", -1,
				axp_rtc_resource, 2);
}

#ifdef CONFIG_SATA_MV
#define SATA_PHYS_BASE (INTER_REGS_PHYS_BASE | 0xA0000)
#define IRQ_DSMP_SATA IRQ_AURORA_SATA0

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
		.start	= IRQ_DSMP_SATA,
		.end	= IRQ_DSMP_SATA,
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
	if (mvUnitMapIsMine(SATA) != MV_TRUE)
		return;

	armadaxp_sata.dev.platform_data = sata_data;
	sata_data->dram = &armadaxp_mbus_dram_info;
	platform_device_register(&armadaxp_sata);
}
#endif
 
void __init armadaxp_hwmon_init(void)
{
	if (mvUnitMapIsMine(HWMON) == MV_TRUE)
	platform_device_register_simple("axp-temp", 0, NULL, 0);
}

static struct timer_list axp_db_timer;
static void axp_db_7seg_event(unsigned long data)
{
	static int count = 0;

	mvBoardDebugLed(count);

	count = (count + 1) & 7;
	mod_timer(&axp_db_timer, jiffies + 1 * HZ);
}

static int __init axp_db_7seg_init(void)
{
	 
	setup_timer(&axp_db_timer, axp_db_7seg_event, 0);

	mod_timer(&axp_db_timer, jiffies + 1 * HZ);

	return 0;
}
__initcall(axp_db_7seg_init);

#ifdef CONFIG_FB_DOVE
 
static struct dovefb_mach_info kw_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.lcd_ref_clk		= 25000000,
#if defined(CONFIG_FB_DOVE_CLCD_DCONB_BYPASS0)
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
#else
	.io_pin_allocation      = IOPAD_DUMB24,
	.panel_rgb_type         = DUMB24_RGB888_0,
#endif
	.panel_rgb_reverse_lanes = 0,
	.gpio_output_data	= 3,
	.gpio_output_mask	= 3,
	.ddc_polling_disable	= 1,
	.ddc_i2c_address	= 0x50,
	.ddc_i2c_adapter	= 0,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
	.lvds_info = {
		.lvds_24b_option = 1,
		.lvds_tick_drv = 2
	}
};

static struct dovefb_mach_info kw_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes = 0,
	.gpio_output_data	= 3,
	.gpio_output_mask	= 3,
	.ddc_i2c_adapter	= -1,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 0,
	.active			= 1,
	.enable_lcd0		= 0,
};

static struct dovebl_platform_data dsmp_backlight_data = {
	.default_intensity = 0xa,
	.gpio_pm_control = 1,

	.lcd_start = LCD_PHYS_BASE,	 
	.lcd_end = LCD_PHYS_BASE+0x1C8,	 
	.lcd_offset = LCD_SPU_DUMB_CTRL, 
	.lcd_mapped = 0,		 
	.lcd_mask = 0x0,		 
	.lcd_on = 0x0,			 
	.lcd_off = 0x0,			 

	.blpwr_start = LCD_PHYS_BASE,  
	.blpwr_end = LCD_PHYS_BASE+0x1C8, 
	.blpwr_offset = LCD_SPU_DUMB_CTRL, 
	.blpwr_mapped = 0,		 
	.blpwr_mask = 0x0,		 
	.blpwr_on = 0x0,		 
	.blpwr_off = 0x0,		 

	.btn_start = LCD_PHYS_BASE,  
	.btn_end = LCD_PHYS_BASE+0x1C8,	 
	.btn_offset = LCD_CFG_GRA_PITCH,	 
	.btn_mapped = 0,		 
	.btn_mask = 0xF0000000,	 
	.btn_level = 15,	 
	.btn_min = 0x1,	 
	.btn_max = 0xF,	 
	.btn_inc = 0x1,	 
};

#endif  

#ifdef CONFIG_MTD_NAND_NFC
 
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
	if (mvUnitMapIsMine(NAND) != MV_TRUE)
		return;

	if (nfcConfig) {
		if (strncmp(nfcConfig, "ganged", 6) == 0) {
			axp_nfc_data.nfc_width = 16;
			axp_nfc_data.num_devs = 2;
			nfcConfig += 7;
		}

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
	printk("\n");
	printk("  Marvell Armada-XP");

	mvBoardNameGet(name_buff);
	printk(" %s Board - ",name_buff);

	mvCtrlModelRevNameGet(name_buff);
	printk(" Soc: %s",  name_buff);
#if defined(MV_CPU_LE)
	printk(" LE\n");
#else
	printk(" BE\n");
#endif
	printk("  Detected Tclk %d, SysClk %d, FabricClk %d, PClk %d\n",mvTclk, mvSysclk, mvCpuL2ClkGet(), mvCpuPclkGet());
	printk("  LSP version: %s\n", LSP_VERSION);
#ifdef CONFIG_MV_AMP_ENABLE
	mvUnitMapPrint();
#endif
	printk("\n");
}

#ifdef	CONFIG_AURORA_IO_CACHE_COHERENCY
static void io_coherency_init(void)
{
	MV_U32 reg;

	reg = MV_REG_READ(MV_CIB_CTRL_CFG_REG);
	reg &= ~(7 << 16);
	reg |= (7 << 16);
	MV_REG_WRITE(MV_CIB_CTRL_CFG_REG, reg);

#ifndef CONFIG_SMP
         
	reg = MV_REG_READ(MV_COHERENCY_FABRIC_CTRL_REG);
	reg &= ~(0x3<<24);
	reg |= 1<<24;
	MV_REG_WRITE(MV_COHERENCY_FABRIC_CTRL_REG, reg);

	reg = MV_REG_READ(MV_COHERENCY_FABRIC_CFG_REG);
	reg &= ~(0x3<<24);
	reg |= 1<<24;
	MV_REG_WRITE(MV_COHERENCY_FABRIC_CFG_REG, reg);
#endif
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
	printk("Aurora: Working in ARMv%d mode\n",cpu_mode);
}
#endif

static struct mv_xor_platform_shared_data armadaxp_xor_shared_data = {
	.dram		= &armadaxp_mbus_dram_info,
};

static u64 armadaxp_xor_dmamask = DMA_BIT_MASK(32);

#ifdef XOR0_ENABLE
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
		.start	= IRQ_AURORA_XOR00,
		.end	= IRQ_AURORA_XOR00,
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
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &armadaxp_xor00_data,
	},
};

static struct resource armadaxp_xor01_resources[] = {
	[0] = {
		.start	= IRQ_AURORA_XOR01,
		.end	= IRQ_AURORA_XOR01,
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
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &armadaxp_xor01_data,
	},
};

static void __init armadaxp_xor0_init(void)
{
	if (mvUnitMapIsMine(XOR0) != MV_TRUE)
		return;

	platform_device_register(&armadaxp_xor0_shared);

	dma_cap_set(DMA_MEMCPY, armadaxp_xor00_data.cap_mask);
	dma_cap_set(DMA_XOR, armadaxp_xor00_data.cap_mask);
	platform_device_register(&armadaxp_xor00_channel);

	dma_cap_set(DMA_MEMCPY, armadaxp_xor01_data.cap_mask);
	dma_cap_set(DMA_MEMSET, armadaxp_xor01_data.cap_mask);
	dma_cap_set(DMA_XOR, armadaxp_xor01_data.cap_mask);
	platform_device_register(&armadaxp_xor01_channel);
}
#endif

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
		.start	= IRQ_AURORA_XOR10,
		.end	= IRQ_AURORA_XOR10,
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
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &armadaxp_xor10_data,
	},
};

static struct resource armadaxp_xor11_resources[] = {
	[0] = {
		.start	= IRQ_AURORA_XOR11,
		.end	= IRQ_AURORA_XOR11,
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
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &armadaxp_xor11_data,
	},
};

static void __init armadaxp_xor1_init(void)
{
	if (mvUnitMapIsMine(XOR1) != MV_TRUE)
		return;

	platform_device_register(&armadaxp_xor1_shared);

	dma_cap_set(DMA_XOR, armadaxp_xor10_data.cap_mask);
	platform_device_register(&armadaxp_xor10_channel);

	dma_cap_set(DMA_MEMCPY, armadaxp_xor11_data.cap_mask);
	dma_cap_set(DMA_MEMSET, armadaxp_xor11_data.cap_mask);
	 
	platform_device_register(&armadaxp_xor11_channel);
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

#ifdef CONFIG_MV_AMP_ENABLE
static int mvAmpInitCpuIf()
{
	if(mvUnitMapIsMine(MSTR) == MV_TRUE){

		if(mvReadAmpReg(ADR_WIN_EN_REG) != 0)
			printk("Warning: AMP Address decode windows reg != 0\n");

		if (mvCpuIfInit(mv_sys_map()) != MV_OK)
			return 1;

		mvWriteAmpReg(ADR_WIN_EN_REG, 1);
		printk("Initialized Address decode windows\n");
	}
	else
	{
		 
		while(mvReadAmpReg(ADR_WIN_EN_REG) == 0){
			udelay(1);
		}

		if(mvCpuIfVerify(mv_sys_map()) != MV_OK)
			return 1;

		printk("Verified Address decode windows\n");
	}

	return 0;
}
#endif

void axp_db_restore(void)
{
	int maxPorts, port;
#ifdef CONFIG_MV_INCLUDE_SDIO
	static MV_UNIT_WIN_INFO addrWinMap[MAX_TARGETS + 1];
#endif

	mvBoardEnvInit();

	if (mvCtrlEnvInit()) {
		pr_warn("Controller env initialization failed.\n");
		return;
	}

	if (mvCpuIfInit(mv_sys_map())) {
		pr_warn("Cpu Interface initialization failed.\n");
		return;
	}

#ifdef CONFIG_MV_INCLUDE_SDIO
	if(MV_OK == mvCtrlAddrWinMapBuild(addrWinMap, MAX_TARGETS + 1))
		if (MV_OK == mvSdmmcWinInit(addrWinMap)) {
			printk("Resuming SDIO\n");
		}
#endif

#ifdef CONFIG_MV_INCLUDE_USB
	mvUsbPllInit();
#endif
	maxPorts =  mvCtrlEthMaxPortGet();

	for (port = 0; port < maxPorts; port++)
		mvEthPhyInit(port, MV_FALSE);

	
	axp_timer_resume();
}

#ifdef CONFIG_SYNO_ARMADA_ARCH
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
	 
	syno_mv_net_shutdown();
#endif
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_SHUTDOWN, UART1_REG(TX));
}

static void synology_restart(char mode, const char *cmd)
{
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_REBOOT, UART1_REG(TX));

        arm_machine_restart(mode, cmd);
}
#endif  

static void __init axp_db_init(void)
{
#ifdef CONFIG_MV_AMP_ENABLE
	 
	if(mvUnitMapIsRsrcLimited() == MV_FALSE)
		mvUnitMapSetAllMine();
#endif

	
	cpu_fabric_common_init();

	
#ifndef CONFIG_SYNO_ARMADA_ARCH
	 
#if defined(CONFIG_ARMADA_XP_REV_A0) || defined(CONFIG_ARMADA_XP_REV_B0)
	gBoardId = DB_88F78XX0_BP_REV2_ID;
#else
	gBoardId = DB_88F78XX0_BP_ID;
#endif
#endif

	mvBoardEnvInit();

	if( mvCtrlEnvInit() ) {
		printk( "Controller env initialization failed.\n" );
		return;
	}

	armadaxp_setup_cpu_mbus();

#ifdef CONFIG_MV_AMP_ENABLE
	if(mvAmpInitCpuIf()){
#else
	if(mvCpuIfInit(mv_sys_map())) {
#endif
		printk( "Cpu Interface initialization failed.\n" );
		return;
	}

	mvTclk = mvBoardTclkGet();
	mvSysclk = mvBoardSysClkGet();

	elf_hwcap &= ~HWCAP_JAVA;

#ifndef CONFIG_MV_UART_PORT
	serial_initialize(0);
#else
	serial_initialize(CONFIG_MV_UART_PORT);
#endif
#ifdef CONFIG_SYNO_ARMADA_ARCH
	serial_initialize(1);
#endif

	print_board_info();

	mv_gpio_init();

		rtc_init();

#ifdef CONFIG_MV_INCLUDE_SPI
	 
	if(mvUnitMapIsMine(SPI) == MV_TRUE)
		mvSysSpiInit(0, _16M);
#endif

	mvSysEthPhyInit();

#ifdef CONFIG_SATA_MV
		armadaxp_sata_init(&dbdsmp_sata_data);
#endif
#ifdef CONFIG_MTD_NAND_NFC
	 
		axp_db_nfc_init();
#endif
	 
		armadaxp_hwmon_init();

#ifdef XOR0_ENABLE
		armadaxp_xor0_init();
#endif
		armadaxp_xor1_init();

	armadaxp_i2c0_init();

#ifdef CONFIG_FB_DOVE
	if ((lcd0_enable == 1) && (lcd_panel == 0))
		armadaxp_i2c1_init();
#endif
	 
#if defined(CONFIG_MV_INCLUDE_SDIO)
	armadaxp_sdio_init();
#endif

#ifdef CONFIG_MV_ETHERNET
	 
	eth_init();
#endif

#ifdef CONFIG_MV_IPC_NET
	platform_device_register(&mv_ipc_net);
#endif

#ifdef CONFIG_FB_DOVE
	if(mvUnitMapIsMine(LCD) == MV_TRUE){
		kw_lcd0_dmi.dram = &armadaxp_mbus_dram_info;
		if (lcd_panel) {
			kw_lcd0_dmi.lvds_info.enabled = 1;
			kw_lcd0_dmi.fixed_full_div = 1;
			kw_lcd0_dmi.full_div_val = 7;
	 
			printk(KERN_INFO "LCD Panel enabled.\n");
		}
		clcd_platform_init(&kw_lcd0_dmi, &kw_lcd0_vid_dmi, &dsmp_backlight_data);
	}
#endif

#if defined(CONFIG_SYNO_ARMADA_ARCH)
	pm_power_off = synology_power_off;
	arm_pm_restart = synology_restart;
	synology_gpio_init();
#endif
	return;
}

#ifdef CONFIG_FB_DOVE
 
void __init axp_tag_fixup_mem32(struct tag *t, char **cmdline, struct meminfo *mi)

{
	struct tag *last_tag = NULL;
	int total_size = PAGE_ALIGN(DEFAULT_FB_SIZE) * 2;
	uint32_t memory_start;

	for (; read_tag(t->hdr.size); t = tag_next(t))
		if ((read_tag(t->hdr.tag) == ATAG_MEM) && ( read_tag(t->u.mem.size) >= total_size)) {
			if ((last_tag == NULL) ||
			    (read_tag(t->u.mem.start) > last_tag->u.mem.start))
				last_tag = t;
		}

	if (last_tag == NULL) {
		early_printk(KERN_WARNING "No suitable memory tag was found, "
				"required memory %d MB.\n", total_size);
		return;
	}

	last_tag->u.mem.size -= total_size;
	memory_start = last_tag->u.mem.start + last_tag->u.mem.size + 1;

	kw_lcd0_dmi.fb_mem[0] = (void*)memory_start;
	kw_lcd0_dmi.fb_mem_size[0] = total_size / 2;
	kw_lcd0_dmi.fb_mem[1] = (void*)(memory_start + kw_lcd0_dmi.fb_mem_size[0]);
	kw_lcd0_dmi.fb_mem_size[1] = total_size / 2;

}
#endif  

#ifdef CONFIG_SUSPEND
#define TRAINING_SPACE	(10*1024)

void __init reserve_training_mem(void)
{
	int i;
	MV_UNIT_WIN_INFO addr_win_map[MAX_TARGETS + 1];
	phys_addr_t base;
	phys_addr_t size = (phys_addr_t)(TRAINING_SPACE);
	struct map_desc  early_mem_table;

	early_mem_table.virtual = INTER_REGS_BASE;
	early_mem_table.pfn     = __phys_to_pfn(INTER_REGS_PHYS_BASE);
	early_mem_table.length  = SZ_1M;
	early_mem_table.type    = MT_DEVICE;

	iotable_init(&early_mem_table , 1);

	mvCtrlAddrWinMapBuild(addr_win_map, MAX_TARGETS + 1);
	for (i = 0; i < MAX_TARGETS; i++) {
		if (!MV_TARGET_IS_DRAM(i))
			continue;

		if (addr_win_map[i].enable == MV_FALSE)
			continue;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
		base  = ((phys_addr_t)addr_win_map[i].addrWin.baseHigh) << 32;
#else
		base = 0;
#endif
		base |=  addr_win_map[i].addrWin.baseLow;

		pr_info("Reserving training memory: base=0x%p size=0x%x\n",
				(void *)base, size);

		memblock_reserve(base, size);
	}
}
#endif

MACHINE_START(ARMADA_XP_DB, "Marvell Armada XP Development Board")
	 
	.atag_offset	= BOOT_PARAMS_OFFSET,
	.map_io		= axp_map_io,
	.init_irq	= axp_init_irq,
	.timer		= &axp_timer,
	.init_machine	= axp_db_init,
#ifdef CONFIG_FB_DOVE
	 
	.fixup		= axp_tag_fixup_mem32,
#endif  
#ifdef CONFIG_SUSPEND
	.reserve	= reserve_training_mem,
#endif  
MACHINE_END

static void __init axp_gp_init(void)
{
#ifdef CONFIG_MV_AMP_ENABLE
	 
	if (mvUnitMapIsRsrcLimited() == MV_FALSE)
		mvUnitMapSetAllMine();
#endif

	cpu_fabric_common_init();

	gBoardId = DB_784MP_GP_ID;

	mvBoardEnvInit();

	if( mvCtrlEnvInit() ) {
		printk( "Controller env initialization failed.\n" );
		return;
	}

	armadaxp_setup_cpu_mbus();

#ifdef CONFIG_MV_AMP_ENABLE
	if (mvAmpInitCpuIf()) {
#else
	if (mvCpuIfInit(mv_sys_map())) {
#endif
		printk( "Cpu Interface initialization failed.\n" );
		return;
	}

	mvTclk = mvBoardTclkGet();
	mvSysclk = mvBoardSysClkGet();

	elf_hwcap &= ~HWCAP_JAVA;

#ifndef CONFIG_MV_UART_PORT
	serial_initialize(0);
#else
	serial_initialize(CONFIG_MV_UART_PORT);
#endif

	print_board_info();

	mv_gpio_init();

	rtc_init();

#ifdef CONFIG_MV_INCLUDE_SPI
	 
	if (mvUnitMapIsMine(SPI) == MV_TRUE)
	mvSysSpiInit(0, _16M);
#endif

	mvSysEthPhyInit();

#ifdef CONFIG_SATA_MV
	armadaxp_sata_init(&dbdsmp_sata_data);
#endif
#ifdef CONFIG_MTD_NAND_NFC
	 
	axp_db_nfc_init();
#endif
	 
	armadaxp_hwmon_init();

#ifdef XOR0_ENABLE
	armadaxp_xor0_init();
#endif
	armadaxp_xor1_init();

	armadaxp_i2c0_init();

#ifdef CONFIG_FB_DOVE
	if ((lcd0_enable == 1) && (lcd_panel == 0))
		armadaxp_i2c1_init();
#endif

#if defined(CONFIG_MV_INCLUDE_SDIO)
	armadaxp_sdio_init();
#endif

#ifdef CONFIG_MV_ETHERNET
	 
	eth_init();
#endif

#ifdef CONFIG_MV_IPC_NET
	platform_device_register(&mv_ipc_net);
#endif

#ifdef CONFIG_FB_DOVE
	kw_lcd0_dmi.dram = &armadaxp_mbus_dram_info;
	if (lcd_panel) {
		kw_lcd0_dmi.lvds_info.enabled = 1;
		kw_lcd0_dmi.fixed_full_div = 1;
		kw_lcd0_dmi.full_div_val = 7;
 
		printk(KERN_INFO "LCD Panel enabled.\n");
	}
	clcd_platform_init(&kw_lcd0_dmi, &kw_lcd0_vid_dmi, &dsmp_backlight_data);
#endif

	return;
}

MACHINE_START(ARMADA_XP_GP, "Marvell Armada XP GP Board")
	 
	.atag_offset	= 0x00000100,
	.map_io		= axp_map_io,
	.init_irq	= axp_init_irq,
	.timer		= &axp_timer,
	.init_machine	= axp_gp_init,
#ifdef CONFIG_FB_DOVE
	 
	.fixup		= axp_tag_fixup_mem32,
#endif  
#ifdef CONFIG_SUSPEND
	.reserve	= reserve_training_mem,
#endif  
MACHINE_END

static void __init axp_amc_init(void)
{
#ifdef CONFIG_MV_AMP_ENABLE
	 
	if (mvUnitMapIsRsrcLimited() == MV_FALSE)
		mvUnitMapSetAllMine();
#endif

	cpu_fabric_common_init();

	gBoardId = DB_78X60_AMC_ID;

	mvBoardEnvInit();

	if (mvCtrlEnvInit()) {
		printk(KERN_ERR "Controller env initialization failed.\n");
		return;
	}

	armadaxp_setup_cpu_mbus();

#ifdef CONFIG_MV_AMP_ENABLE
	if (mvAmpInitCpuIf()) {
#else
	if (mvCpuIfInit(mv_sys_map())) {
#endif
		printk(KERN_ERR "Cpu Interface initialization failed.\n");
		return;
	}

	mvTclk = mvBoardTclkGet();
	mvSysclk = mvBoardSysClkGet();

	elf_hwcap &= ~HWCAP_JAVA;

#ifndef CONFIG_MV_UART_PORT
	serial_initialize(0);
#else
	serial_initialize(CONFIG_MV_UART_PORT);
#endif

	print_board_info();

	mv_gpio_init();

	rtc_init();

#ifdef CONFIG_MV_INCLUDE_SPI
	 
	if (mvUnitMapIsMine(SPI) == MV_TRUE)
		mvSysSpiInit(0, _16M);
#endif

	mvSysEthPhyInit();

#ifdef CONFIG_SATA_MV
	armadaxp_sata_init(&dbdsmp_sata_data);
#endif
#ifdef CONFIG_MTD_NAND_NFC
	 
	axp_db_nfc_init();
#endif
	 
	armadaxp_hwmon_init();

#ifdef XOR0_ENABLE
	armadaxp_xor0_init();
#endif
	armadaxp_xor1_init();

	armadaxp_i2c0_init();

#ifdef CONFIG_FB_DOVE
	if ((lcd0_enable == 1) && (lcd_panel == 0))
		armadaxp_i2c1_init();
#endif
	 
#if defined(CONFIG_MV_INCLUDE_SDIO)
	armadaxp_sdio_init();
#endif

#ifdef CONFIG_MV_ETHERNET
	 
	eth_init();
#endif

#ifdef CONFIG_MV_IPC_NET
	platform_device_register(&mv_ipc_net);
#endif

#ifdef CONFIG_FB_DOVE
	if (mvUnitMapIsMine(LCD) == MV_TRUE) {
		kw_lcd0_dmi.dram = &armadaxp_mbus_dram_info;
		if (lcd_panel) {
			kw_lcd0_dmi.lvds_info.enabled = 1;
			kw_lcd0_dmi.fixed_full_div = 1;
			kw_lcd0_dmi.full_div_val = 7;
			 
			printk(KERN_INFO "LCD Panel enabled.\n");
		}
		clcd_platform_init(&kw_lcd0_dmi, &kw_lcd0_vid_dmi,
						&dsmp_backlight_data);
	}
#endif

	return;
}

MACHINE_START(ARMADA_XP_AMC, "Marvell Armada XP AMC Board")
	 
	.atag_offset	= 0x00000100,
	.map_io		= axp_map_io,
	.init_irq	= axp_init_irq,
	.timer		= &axp_timer,
	.init_machine	= axp_amc_init,
#ifdef CONFIG_FB_DOVE
	 
	.fixup		= axp_tag_fixup_mem32,
#endif  
#ifdef CONFIG_SUSPEND
	.reserve	= reserve_training_mem,
#endif  
MACHINE_END

static void __init axp_rd_nas_init(void)
{
	 
	cpu_fabric_common_init();

	gBoardId = RD_78460_NAS_ID;

	mvBoardEnvInit();

	if( mvCtrlEnvInit() ) {
		printk( "Controller env initialization failed.\n" );
		return;
	}

	armadaxp_setup_cpu_mbus();

	if( mvCpuIfInit(mv_sys_map())) {
		printk( "Cpu Interface initialization failed.\n" );
		return;
	}

	mvTclk = mvBoardTclkGet();
	mvSysclk = mvBoardSysClkGet();

	elf_hwcap &= ~HWCAP_JAVA;

	serial_initialize(0);

	print_board_info();

	mv_gpio_init();

	rtc_init();

#ifdef CONFIG_MV_INCLUDE_SPI
	 
	mvSysSpiInit(0, _16M);
#endif

	mvSysEthPhyInit();

#ifdef CONFIG_SATA_MV
	armadaxp_sata_init(&dbdsmp_sata_data);
#endif
#ifdef CONFIG_MTD_NAND_NFC
	 
	axp_db_nfc_init();
#endif
	 
	armadaxp_hwmon_init();

#ifdef XOR0_ENABLE
	armadaxp_xor0_init();
#endif
	armadaxp_xor1_init();

	armadaxp_i2c0_init();

#ifdef CONFIG_FB_DOVE
	if ((lcd0_enable == 1) && (lcd_panel == 0))
		armadaxp_i2c1_init();
#endif

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
	 
	eth_init();
#endif

#ifdef CONFIG_FB_DOVE
	kw_lcd0_dmi.dram = &armadaxp_mbus_dram_info;
	if (lcd_panel) {
		kw_lcd0_dmi.lvds_info.enabled = 1;
		kw_lcd0_dmi.fixed_full_div = 1;
		kw_lcd0_dmi.full_div_val = 7;
 
		printk(KERN_INFO "LCD Panel enabled.\n");
	}
	clcd_platform_init(&kw_lcd0_dmi, &kw_lcd0_vid_dmi, &dsmp_backlight_data);
#endif

	return;
}

MACHINE_START(ARMADA_XP_RD_NAS, "Marvell Armada XP RD NAS Board")
	 
	.atag_offset	= 0x00000100,
	.map_io		= axp_map_io,
	.init_irq	= axp_init_irq,
	.timer		= &axp_timer,
	.init_machine	= axp_rd_nas_init,
#ifdef CONFIG_FB_DOVE
	 
	.fixup		= axp_tag_fixup_mem32,
#endif  
MACHINE_END

static void __init axp_rdsrv_init(void)
{
	 
	cpu_fabric_common_init();

#if defined(CONFIG_ARMADA_XP_REV_A0) || defined(CONFIG_ARMADA_XP_REV_B0)
	gBoardId = RD_78460_SERVER_ID;
#else
	gBoardId = RD_78460_SERVER_ID;
#endif

	mvBoardEnvInit();

	if( mvCtrlEnvInit() ) {
		printk( "Controller env initialization failed.\n" );
		return;
	}

	armadaxp_setup_cpu_mbus();

	if( mvCpuIfInit(mv_sys_map())) {
		printk( "Cpu Interface initialization failed.\n" );
		return;
	}

	mvTclk = mvBoardTclkGet();
	mvSysclk = mvBoardSysClkGet();

	elf_hwcap &= ~HWCAP_JAVA;

	serial_initialize(0);

	print_board_info();

	mv_gpio_init();

	rtc_init();

	mvSysSpiInit(0, _16M);

	mvSysEthPhyInit();

#ifdef CONFIG_SATA_MV
	armadaxp_sata_init(&dbdsmp_sata_data);
#endif

	armadaxp_hwmon_init();

	armadaxp_i2c0_init();

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
	 
	eth_init();
#endif

	return;
}

MACHINE_START(ARMADA_XP_RDSRV, "Marvell Armada XP Server Board")
		 
	.atag_offset	= BOOT_PARAMS_OFFSET,
	 .map_io	= axp_map_io,
  	.init_irq	= axp_init_irq,
  	.timer		= &axp_timer,
  	.init_machine	= axp_rdsrv_init,
  MACHINE_END

extern MV_TARGET_ATTRIB mvTargetDefaultsArray[];
static void __init axp_fpga_init(void)
{
	 
	cpu_fabric_common_init();

	gBoardId = FPGA_88F78XX0_ID;

       	mvBoardEnvInit();

        if( mvCtrlEnvInit() ) {
            printk( "Controller env initialization failed.\n" );
            return;
        }
	
	mvTargetDefaultsArray[PEX0_MEM].attrib = 0xD8;

	armadaxp_setup_cpu_mbus();

	if( mvCpuIfInit(mv_sys_map())) {
		printk( "Cpu Interface initialization failed.\n" );
		return;
	}

    	mvTclk = mvBoardTclkGet();
   	mvSysclk = mvBoardSysClkGet();

	elf_hwcap &= ~HWCAP_JAVA;

	serial_initialize(0);

	print_board_info();

	mv_gpio_init();

	rtc_init();

	return;
}

MACHINE_START(ARMADA_XP_FPGA, "Marvell Armada XP FPGA Board")
	.atag_offset	= 0x00000100,
	.map_io		= axp_map_io,
	.init_irq	= axp_init_irq,
	.timer		= &axp_timer,
	.init_machine	= axp_fpga_init,
MACHINE_END
