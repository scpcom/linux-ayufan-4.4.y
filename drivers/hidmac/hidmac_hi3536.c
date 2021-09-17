#include <linux/kernel.h>
#include <linux/io.h>

#include <mach/platform.h>
#include <mach/io.h>

#include "hidmac_hi3536.h"

#define IO_SYS_CRG_BASE    IO_ADDRESS(REG_BASE_CRG)

/*
 *	DMAC channel request default configure array
 * Request ID, peripheral data register address, Control, Config, width
 */
dmac_peripheral g_peripheral[DMAC_MAX_PERIPHERALS] = {
	/* Request 0: UART0 Rx 8bit width */
	{ 0, UART0_DATA_REG, 0x99000000, 0xd000, 0},

	/* Request 1: UART0 Tx 8bit width */
	{ 1, UART0_DATA_REG, 0x96000000, 0xc840, 0},

	/* Request 2: UART1 Rx 8bit width */
	{ 2, UART1_DATA_REG, 0x99000000, 0xd004, 0},

	/* Request 3: UART1 Tx 8bit width */
	{ 3, UART1_DATA_REG, 0x96000000, 0xc8c0, 0},

	/* Request 4: UART2 Rx 8bit width */
	{ 4, UART2_DATA_REG, 0x99000000, 0xd008, 0},

	/* Request 5: UART2 Tx 8bit width */
	{ 5, UART2_DATA_REG, 0x96000000, 0xc940, 0},

	/* Request 6: UART3 Rx 8bit width */
	{ 6, UART3_DATA_REG, 0x99000000, 0xd00c, 0},

	/* Request 7: UART3 Tx 8bit width */
	{ 7, UART3_DATA_REG, 0x96000000, 0xc9c0, 0},

	/* Request 8: I2C Rx 8bit width */
	{ 8, I2C_DATA_REG, 0x99000000, 0xd010, 0},

	/* Request 9: I2C Tx 8bit width */
	{ 9, I2C_DATA_REG, 0x96000000, 0xca40, 0},
};

void hidmac_clk_en(void)
{
	unsigned int tmp;

	tmp = readl((volatile void *)(IO_SYS_CRG_BASE + REG_DMAC_CRG));
	tmp |= DMAC_CLK_EN;
	writel(tmp, (volatile void *)(IO_SYS_CRG_BASE + REG_DMAC_CRG));
}

void hidmac_unreset(void)
{
	unsigned int tmp;

	tmp = readl((volatile void *)(IO_SYS_CRG_BASE + REG_DMAC_CRG));
	tmp &= ~DMAC_SRST_REQ;
	writel(tmp, (volatile void *)(IO_SYS_CRG_BASE + REG_DMAC_CRG));
}
