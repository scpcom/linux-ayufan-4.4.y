#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irqchip.h>

#include <linux/memblock.h>
#include <linux/antirebootloop.h>

#include <mach/hardware.h>

#include "common.h"

void __init c2k_reserve(void)
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
