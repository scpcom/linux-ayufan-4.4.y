#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvAddrDec.h"

#define AMBA_DEVICE(name, busid, base, plat)	\
struct amba_device name##_device = {		\
	.dev		= {			\
		.coherent_dma_mask = ~0UL,	\
		.init_name = busid,		\
		.platform_data = plat,		\
	},					\
	.res		= {			\
		.start	= base,			\
		.end	= base + SZ_4K - 1,	\
		.flags	= IORESOURCE_MEM,	\
	},					\
	.dma_mask	= ~0UL,			\
	.irq		= IRQ_##base,		\
	/* .dma		= DMA_##base,*/		\
}

void __init mv_usb_init(struct mbus_dram_target_info *dram);

extern void a38x_init_irq(void);
extern void a38x_secondary_startup(void);
extern void __init a38x_map_io(void);
extern void __init set_core_count(unsigned int cpu_count);
extern struct sys_timer a38x_timer;
extern unsigned int elf_hwcap;
extern u32 mvIsUsbHost;
extern MV_CPU_DEC_WIN *mv_sys_map(void);
extern MV_TARGET_ATTRIB mvTargetDefaultsArray[];
MV_STATUS mvSysSpiInit(MV_U8 spiId, MV_U32 serialBaudRate);

#ifdef CONFIG_SMP
extern unsigned int group_cpu_mask;
#else
static unsigned int group_cpu_mask = 1;
#endif

#ifdef MV_INCLUDE_EARLY_PRINTK
extern void putstr(const char *ptr);
#endif

#if defined(CONFIG_MV_INCLUDE_CESA)
extern u32 mv_crypto_virt_base_get(u8 chan);
#endif

#ifdef CONFIG_DEBUG_LL
extern void printascii(const char *);
#endif
