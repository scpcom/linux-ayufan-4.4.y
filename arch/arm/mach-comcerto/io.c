#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>

#include <asm/tlb.h>
#include <asm/mach/map.h>

#include <mach/hardware.h>


static struct map_desc c2k_io_desc[] __initdata = {
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
};

void __init c2k_map_io(void)
{
	iotable_init(c2k_io_desc, ARRAY_SIZE(c2k_io_desc));
}

extern void c2k_clk_div_backup_relocate_table (void);

void __init c2k_init_early(void)
{
	c2k_clk_div_backup_relocate_table();
}
void __init c2k_init_late(void)
{
}
