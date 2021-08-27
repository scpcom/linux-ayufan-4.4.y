 
#ifndef __AL_HAL_NAND_DEFS_H__
#define __AL_HAL_NAND_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif
 
#define AL_NAND_RESET_MASK_SOFT			(1 << 0)

#define AL_NAND_RESET_MASK_CMD_FIFO		(1 << 1)

#define AL_NAND_RESET_MASK_DATA_FIFO		(1 << 2)

#define AL_NAND_RESET_MASK_DDRRX_FIFO		(1 << 3)

#define AL_NAND_RESET_MASK_CMD_ENGINE		(1 << 4)

#define AL_NAND_RESET_MASK_TIMING_ENGINE	(1 << 5)

#define AL_NAND_MAX_NUM_DEVICES	8

#define AL_NAND_INTR_STATUS_CMD_BUF_EMPTY		(1 << 0)
 
#define AL_NAND_INTR_STATUS_CMD_BUF_FULL	        (1 << 1)
 
#define AL_NAND_INTR_STATUS_DATA_BUF_EMPTY	        (1 << 2)
 
#define AL_NAND_INTR_STATUS_DATA_BUF_FULL	        (1 << 3)
 
#define AL_NAND_INTR_STATUS_CORR_ERROR                  (1 << 4)
 
#define AL_NAND_INTR_STATUS_UNCORR_ERROR                (1 << 5)
 
#define AL_NAND_INTR_STATUS_BUF_WRRDY                   (1 << 6)
 
#define AL_NAND_INTR_STATUS_BUF_RDRDY                   (1 << 7)
 
#define AL_NAND_INTR_STATUS_WRRD_DONE                   (1 << 8)
 
#define AL_NAND_INTR_STATUS_DMA_DONE                    (1 << 9)
 
#define AL_NAND_INTR_STATUS_TRANS_COMP                  (1 << 10)
 
#define AL_NAND_INTR_STATUS_CMD_BUF_OVERFLOW            (1 << 11)
 
#define AL_NAND_INTR_STATUS_CMD_BUF_UNDERFLOW           (1 << 12)
 
#define AL_NAND_INTR_STATUS_DATA_BUF_OVERFLOW           (1 << 13)
 
#define AL_NAND_INTR_STATUS_DATA_BUF_UNDERFLOW          (1 << 14)
 
#define AL_NAND_INTR_STATUS_DMA_TRANS_DONE              (1 << 15)
 
#define AL_NAND_INTR_STATUS_DMA_BOUNDARY_CROSS		(1 << 16)
 
#define AL_NAND_INTR_STATUS_SLAVE_ERROR			(1 << 17)

enum al_nand_device_timing_mode {
	AL_NAND_DEVICE_TIMING_MODE_ONFI_0	= 0,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_1	= 1,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_2	= 2,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_3	= 3,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_4	= 4,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_5	= 5,
	AL_NAND_DEVICE_TIMING_MODE_MANUAL	= 6,
};

enum al_nand_device_timing_sdr_read_delay {
	AL_NAND_DEVIE_TIMING_READ_DELAY_1 = 2,
	AL_NAND_DEVIE_TIMING_READ_DELAY_2 = 3,
};

struct al_nand_device_timing {

	uint8_t tSETUP;

	uint8_t tHOLD;

	uint8_t tWH;

	uint8_t tWRP;

	uint8_t tINTCMD;

	uint8_t tRR;

	uint8_t tWB;

	uint8_t readDelay;

	int tCLKDiv;

	int tCE_n;

	int tDQS_in;

	int tDQS_out;
};

enum al_nand_device_sdr_data_width {
	AL_NAND_DEVICE_SDR_DATA_WIDTH_8		= 0,
	AL_NAND_DEVICE_SDR_DATA_WIDTH_16	= 1,	 
};

enum al_nand_device_page_size {
	AL_NAND_DEVICE_PAGE_SIZE_2K	= 0,
	AL_NAND_DEVICE_PAGE_SIZE_4K	= 1,
	AL_NAND_DEVICE_PAGE_SIZE_8K	= 2,
	AL_NAND_DEVICE_PAGE_SIZE_16K	= 3,

	AL_NAND_DEVICE_PAGE_SIZE_512	= 4,
};

struct al_nand_dev_properties {
	enum al_nand_device_timing_mode		timingMode;

	enum al_nand_device_sdr_data_width	sdrDataWidth;

	struct al_nand_device_timing		timing;

	int					readyBusyTimeout;

	int					num_col_cyc;

	int					num_row_cyc;

	enum al_nand_device_page_size		pageSize;
};

enum al_nand_ecc_algorithm {
	AL_NAND_ECC_ALGORITHM_HAMMING	= 0,
	AL_NAND_ECC_ALGORITHM_BCH	= 1,
};

enum al_nand_ecc_bch_num_corr_bits {
	AL_NAND_ECC_BCH_NUM_CORR_BITS_4		= 0,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_8		= 1,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_12	= 2,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_16	= 3,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_20	= 4,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_24	= 5,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_28	= 6,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_32	= 7,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_36	= 8,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_40	= 9,
};

enum al_nand_ecc_bch_message_size {
	AL_NAND_ECC_BCH_MESSAGE_SIZE_512	= 0,
	AL_NAND_ECC_BCH_MESSAGE_SIZE_1024	= 1,
};

struct al_nand_ecc_config {
	enum al_nand_ecc_algorithm		algorithm;

	enum al_nand_ecc_bch_num_corr_bits	num_corr_bits;

	enum al_nand_ecc_bch_message_size	messageSize;

	int					spareAreaOffset;
};

enum al_nand_bad_block_marking_method {
	NAND_BAD_BLOCK_MARKING_METHOD_DISABLED = 0,
	NAND_BAD_BLOCK_MARKING_CHECK_1ST_PAGE,
	NAND_BAD_BLOCK_MARKING_CHECK_1ST_PAGES,
	NAND_BAD_BLOCK_MARKING_CHECK_LAST_PAGE,
	NAND_BAD_BLOCK_MARKING_CHECK_LAST_PAGES,
};

struct al_nand_bad_block_marking {
	enum al_nand_bad_block_marking_method	method;
	int					location1;
	int					location2;
};

struct al_nand_extra_dev_properties {
	unsigned int				pageSize;
	unsigned int				blockSize;
	unsigned int				wordSize;

	struct al_nand_bad_block_marking	badBlockMarking;

	int					eccIsEnabled;
};

struct al_nand_ctrl_obj {
	struct al_nand_regs		*regs_base;

	struct al_nand_wrap_regs	*wrap_regs_base;
	void				*cmd_buff_base;
	void				*data_buff_base;

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	struct al_ssm_dma		*raid_dma;
#else
	struct al_raid_dma		*raid_dma;
#endif
	uint32_t			raid_dma_qid;

	struct al_nand_dev_properties	dev_properties;
	struct al_nand_ecc_config	ecc_config;
	int				current_dev_index;

	uint32_t			cw_size;
	uint32_t			cw_count;

	uint32_t			cw_size_remaining;
	uint32_t			cw_count_remaining;
};

enum al_nand_command_type {
	 
	AL_NAND_COMMAND_TYPE_NOP		= 0,

	AL_NAND_COMMAND_TYPE_CMD		= 2,

	AL_NAND_COMMAND_TYPE_ADDRESS		= 3,

	AL_NAND_COMMAND_TYPE_WAIT_CYCLE_COUNT	= 4,

	AL_NAND_COMMAND_TYPE_WAIT_FOR_READY	= 5,

	AL_NAND_COMMAND_TYPE_DATA_READ_COUNT	= 6,

	AL_NAND_COMMAND_TYPE_DATA_WRITE_COUNT	= 7,

	AL_NAND_COMMAND_TYPE_STATUS_READ	= 8,

	AL_NAND_COMMAND_TYPE_SPARE_READ_COUNT	= 9,

	AL_NAND_COMMAND_TYPE_SPARE_WRITE_COUNT	= 10,

	AL_NAND_COMMAND_TYPE_STATUS_WRITE	= 11,
};

struct al_nand_command {
	enum al_nand_command_type	type;
	uint8_t				argument;
};

#ifdef __cplusplus
}
#endif
 
#endif		 
