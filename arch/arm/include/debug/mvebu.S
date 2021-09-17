#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#if defined(MY_ABC_HERE) && defined(CONFIG_DEBUG_MVEBU_UART_ALTERNATE)
#define ARMADA_370_XP_REGS_PHYS_BASE	0xf1000000
#else
#define ARMADA_370_XP_REGS_PHYS_BASE	0xd0000000
#endif

#define ARMADA_370_XP_REGS_VIRT_BASE	0xfec00000

	.macro	addruart, rp, rv, tmp
	ldr	\rp, =ARMADA_370_XP_REGS_PHYS_BASE
	ldr	\rv, =ARMADA_370_XP_REGS_VIRT_BASE
	orr	\rp, \rp, #0x00012000
	orr	\rv, \rv, #0x00012000
	.endm

#define UART_SHIFT	2
#include <asm/hardware/debug-8250.S>
