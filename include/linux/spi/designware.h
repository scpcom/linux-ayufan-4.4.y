/*
 * designware.h - platform glue for the Synopsys DesignWare SPI controller
 */

#define        CLK_NAME       10
#define        TX_FIFO_DEPTH  8
#define        RX_FIFO_DEPTH  8

struct spi_controller_pdata {
	int use_dma;
	int num_chipselects;
	int bus_num;
	u32 max_freq;
	char clk_name[CLK_NAME];
};
