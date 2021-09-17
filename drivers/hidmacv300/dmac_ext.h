#ifndef __DMAC_EXT_H__
#define __DMAC_EXT_H__

#define DMAC_UART_RX_REQ	12
#define DMAC_UART_TX_REQ	13
#define DMAC_SSP_RX_REQ		14
#define DMAC_SSP_TX_REQ		15

struct dmac_user_para {
	unsigned int src;
	unsigned int dst;
	unsigned int size;
	unsigned int num;
};

extern int hi_memcpy(void *dst, const void *src, size_t count);

#endif
