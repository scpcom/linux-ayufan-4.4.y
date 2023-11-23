// not working yet so disabled. (setup via stty and sending just the led cmd doesn't work either)

#if 0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>

#include <mach/comcerto-2000/clk-rst.h>

#include "boxreset-uart.h"

/* we can't rely on anything during a panic, so reset the uart and write to it directly */

//////////////// GPIO SETUP //////////////////////

#define c2k_readl(a) readl((void *)(a))
#define c2k_writel(v, a) writel((v), (void *)(a))

// why is GPIO_8 defined as 3 << 16 for comcerto ?
#define C2K_GPIO_8 0x00000100
#define C2K_GPIO_9 0x00000200

#define GPIO8_UART0_RX		(0x2 << 16)
#define GPIO9_UART0_TX		(0x2 << 18)
#define UART0_BUS	(GPIO8_UART0_RX | GPIO9_UART0_TX)

void c2k_init_uart_led(void)
{
    // select alternate function for GPIO 8 and 9
    c2k_writel((c2k_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(C2K_GPIO_8 | C2K_GPIO_9)) | UART0_BUS, COMCERTO_GPIO_PIN_SELECT_REG);
    
    // disable output enable GPIO 8 (i.e. set as input)
    c2k_writel(c2k_readl(COMCERTO_GPIO_OE_REG) & ~C2K_GPIO_8, COMCERTO_GPIO_OE_REG);

    // enable  output enable GPIO 9
    c2k_writel(c2k_readl(COMCERTO_GPIO_OE_REG) | C2K_GPIO_9, COMCERTO_GPIO_OE_REG);
    
    // set GPIO 9 to 0
    c2k_writel(c2k_readl(COMCERTO_GPIO_OUTPUT_REG) & ~C2K_GPIO_9, COMCERTO_GPIO_OUTPUT_REG);
}

//////////////// CLOCK FREQUENCY //////////////////////

unsigned long HAL_get_clk_freq(u32 ctrl_reg, u32 div_reg);

unsigned HAL_get_axi_clk(void)
{   
    return HAL_get_clk_freq(AXI_CLK_CNTRL_0, AXI_CLK_DIV_CNTRL);
}

//////////////// SET BAUDRATE //////////////////////

#define LCR_DLAB        0x80
#define UART_LCR        0x0C
#define UART_DLL        0x00
#define UART_DLH        0x04
#define LCR_CHAR_LEN_8      0x03
#define FCR_FIFOEN      (1 << 0)
#define FCR_RCVRRES     (1 << 1)
#define FCR_XMITRES     (1 << 2)
#define UART_FCR        0x08

#define UART0_BASEADDR 0x96300000 /* the ledboard */

/* 
 * 16-bit Divisor Latch register that contains the baud rate divisor for the UART.
 *
 * baud rate = (serial clock freq) / (16 * divisor).
 */
void c2k_setbrg(int baudrate)
{
    size_t map_base = UART0_BASEADDR;
    u32 clock = HAL_get_axi_clk();
	u32 div;
    
	/* round to nearest */
	div = (clock + 8 * baudrate) / (16 * baudrate);

	c2k_writel(LCR_DLAB, map_base + UART_LCR); /* Enable Data latch to write divisor latch */

	c2k_writel( (div & 0xFF), map_base + UART_DLL); 
	c2k_writel( (div >> 8 ) & 0xFF, map_base + UART_DLH);

	c2k_writel(0x00, map_base + UART_LCR); /* Disable date latch */
	c2k_writel(LCR_CHAR_LEN_8 , map_base + UART_LCR); /* Eight bits per character, 1 stop bit */
	c2k_writel(FCR_FIFOEN | FCR_RCVRRES | FCR_XMITRES, map_base + UART_FCR); /* Reset Tx and Rx FIFOs; Enable FIFO mode; Set Rx FIFO threshold */
}

//////////////// PUTC //////////////////////

#define UART_LSR    0x14
#define LSR_THRE    0x20
#define UART_THR    0x00

void c2k_putc(unsigned char c)
{
    size_t map_base = UART0_BASEADDR;

	/* wait for room in the tx FIFO on FFUART */
	while ((c2k_readl(map_base + UART_LSR) & LSR_THRE) == 0) ;
	c2k_writel(c, map_base + UART_THR);
}

#endif // 0
