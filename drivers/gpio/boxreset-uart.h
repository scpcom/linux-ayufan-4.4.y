#ifndef _BOX_RESET_UART_H
#define _BOX_RESET_UART_H

void c2k_init_uart_led(void);
void c2k_setbrg(int baudrate);
void c2k_putc(unsigned char c);

#endif // _BOX_RESET_UART_H
