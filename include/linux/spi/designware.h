/*
 * designware.h - platform glue for the Synopsys DesignWare SPI controller
 */

#include <linux/io.h>
#include <linux/scatterlist.h>

#define        CLK_NAME       10
#define        TX_FIFO_DEPTH  8
#define        RX_FIFO_DEPTH  8

/* Register definitions as per "DesignWare DW_apb_ssi Databook", Capture 6. */

#define DWSPI_CTRLR0        0x00
#define DWSPI_CTRLR1        0x04
#define DWSPI_SSIENR        0x08
#define DWSPI_MWCR          0x0C
#define DWSPI_SER           0x10
#define DWSPI_BAUDR         0x14
#define DWSPI_TXFTLR        0x18
#define DWSPI_RXFTLR        0x1C
#define DWSPI_TXFLR         0x20
#define DWSPI_RXFLR         0x24
#define DWSPI_SR            0x28
#define DWSPI_IMR           0x2C
#define DWSPI_ISR           0x30
#define DWSPI_RISR          0x34
#define DWSPI_TXOICR        0x38
#define DWSPI_RXOICR        0x3C
#define DWSPI_RXUICR        0x40
#define DWSPI_ICR           0x44
#define DWSPI_DMACR         0x4C
#define DWSPI_DMATDLR       0x50
#define DWSPI_DMARDLR       0x54
#define DWSPI_IDR           0x58
#define DWSPI_SSI_COMP_VERSION 0x5C
#define DWSPI_DR            0x60

#define DWSPI_CTRLR0_DFS_MASK	0x000f
#define DWSPI_CTRLR0_SCPOL	0x0080
#define DWSPI_CTRLR0_SCPH	0x0040
#define DWSPI_CTRLR0_TMOD_MASK	0x0300

#define DWSPI_SR_BUSY_MASK	0x01
#define DWSPI_SR_TFNF_MASK	0x02
#define DWSPI_SR_TFE_MASK	0x04
#define DWSPI_SR_RFNE_MASK	0x08
#define DWSPI_SR_RFF_MASK	0x10

#define DWSPI_ISR_TXEIS_MASK	0x01
#define DWSPI_ISR_RXFIS_MASK	0x10

#define DWSPI_IMR_TXEIM_MASK	0x01
#define DWSPI_IMR_RXFIM_MASK	0x10

struct spi_controller_pdata {
	int use_dma;
	int num_chipselects;
	int bus_num;
	u32 max_freq;
	char clk_name[CLK_NAME];
};

struct designware_spi;
struct dw_spi_dma_ops {
	int (*dma_init)(struct designware_spi *dws);
	void (*dma_exit)(struct designware_spi *dws);
	int (*dma_transfer)(struct designware_spi *dws, int cs_change);
};

struct designware_spi {
	struct spi_master	*master;
	struct device *dev;
	struct workqueue_struct *workqueue;
	struct work_struct   work;

	struct tasklet_struct pump_transfers;

	struct mutex         lock; /* lock this struct except from queue */
	struct list_head     queue; /* spi_message queue */
	spinlock_t           qlock; /* lock the queue */

	void __iomem	*regs;	/* virt. address of the control registers */
	unsigned long		paddr;
	dma_addr_t		rx_dma;
	dma_addr_t		tx_dma;
	unsigned int		rx_len;
	unsigned int		tx_len;
	unsigned int ssi_clk;	/* clock in Hz */
	unsigned int tx_fifo_depth; /* bytes in TX FIFO */
	unsigned int rx_fifo_depth; /* bytes in RX FIFO */

	u32		irq;

	u8 bits_per_word;	/* current data frame size */
	struct spi_transfer *tx_t; /* current tx transfer */
	struct spi_transfer *rx_t; /* current rx transfer */
	const u8 *tx_ptr;       /* current tx buffer */
	u8 *rx_ptr;             /* current rx buffer */
	int remaining_tx_bytes;	/* bytes left to tx in the current transfer */
	int remaining_rx_bytes;	/* bytes left to rx in the current transfer */
	int status;             /* status of the current spi_transfer */

	struct completion done; /* signal the end of tx for current sequence */

	struct spi_device *spi; /* current spi slave device */
	struct clk	*clk_spi;
	struct list_head *transfers_list; /* head of the current sequence */
	unsigned int tx_count, rx_count; /* bytes in the current sequence */
	size_t			len;

	int			dma_mapped;
	/* Dma info */
	int			dma_inited;
	struct dma_chan		*txchan;
	struct scatterlist	tx_sgl;
	struct dma_chan		*rxchan;
	struct scatterlist	rx_sgl;
	struct device		*dma_dev;
	dma_addr_t		dma_addr; /* phy address of the Data register */
	struct dw_spi_dma_ops	*dma_ops;
	void			*dma_priv; /* platform relate info */
	//RAJIV
	struct spi_message	*cur_msg;

};

