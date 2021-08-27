/*
 * include/asm-arm/arch-aurora/uncompress.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <mach/armada375.h>

#ifndef CONFIG_MV_UART_PORT
#define UART_THR ((unsigned char *)(UART_PHYS_BASE(0) + 0x0))
#define UART_LSR ((unsigned char *)(UART_PHYS_BASE(0) + 0x14))
#else
#define UART_THR ((unsigned char *)(UART_PHYS_BASE(CONFIG_MV_UART_PORT) + 0x0))
#define UART_LSR ((unsigned char *)(UART_PHYS_BASE(CONFIG_MV_UART_PORT) + 0x14))
#endif

#define LSR_THRE	0x20

static void putc(const char c)
{
	int i;

	for (i = 0; i < 0x1000; i++) {
		/* Transmit fifo not full? */
		if (*UART_LSR & LSR_THRE)
			break;
	}

	*UART_THR = c;
}

static void flush(void)
{
	/* empty function */
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
#define arch_decomp_wdog()
