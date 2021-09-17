/* ./driver/hidmacv300/hi_dmacv300.h
 *
 *
 * History:
 *	21-July-2014 create this file
 */
#ifndef __HI_DMAC_H__
#define __HI_DMAC_H__

#define OSDRV_MODULE_VERSION_STRING\
	"HIDMACV300 @Hi3536_OSDRV 2014-07-121 9:45:37"

#define  dmac_writew(addr, value)\
	((*(volatile unsigned int *)(addr)) = (value))

#define  dmac_readw(addr, v)\
	(v = (*(volatile unsigned int *)(addr)))

#define DDRAM_ADRS	0x80000000	/* fixed */
#define DDRAM_SIZE	0x3FFFFFFF	/* 1GB DDR. */

#define FLASH_BASE	0x50000000
#define FLASH_SIZE	0x04000000	/* (32MB) */
#define ITCM_BASE	0x0
#define ITCM_SIZE	0x800

#define DMAC_CONFIGURATIONx_HALT_DMA_ENABLE     (0x01L << 18)
#define DMAC_CONFIGURATIONx_ACTIVE              (0x01L << 17)
#define DMAC_CONFIGURATIONx_CHANNEL_ENABLE       1
#define DMAC_CONFIGURATIONx_CHANNEL_DISABLE      0

#define DMAC_BASE_REG	CONFIG_HIDMAC_BASE_ADDR

/* the definition for DMAC intr status register */
#define DMAC_INTR_STATUS		0x0
#define DMAC_TX_COMPLETE		0x4
#define DMAC_CONFIG_ERROR		0xC
#define DMAC_DATA_TRANS_ERROR		0x10
#define DMAC_TX_COMPLETE_MASK		0x18
#define DMAC_CONFIG_ERROR_MASK		0x20
#define DMAC_DATA_TRANS_ERROR_MASK	0x24
#define DMAC_TX_COMPLETE_RAW		0x600
#define DMAC_CONFIG_ERROR_RAW		0x610
#define DMAC_DATA_ERROR_RAW		0x618
#define DMAC_CHANNEL_PRIORITY		0x688
#define DMAC_CHANNEL_STATUS		0x690
#define DMAC_GLOBLE_CTRL		0x698

/*the definition for DMAC channel register*/
#define DMAC_CHANNEL_BASE	0x800
#define DMAC_CxLENGTH(i)	(DMAC_CHANNEL_BASE + 0x10 + 0x40 * i)
#define DMAC_CxSRCADDR(i)	(DMAC_CHANNEL_BASE + 0x14 + 0x40 * i)
#define DMAC_CxDESTADDR(i)	(DMAC_CHANNEL_BASE + 0x18 + 0x40 * i)
#define DMAC_CxCONFIG(i)	(DMAC_CHANNEL_BASE + 0x1C + 0x40 * i)
#define DMAC_CxAXICONFIG(i)	(DMAC_CHANNEL_BASE + 0x20 + 0x40 * i)
#define DMAC_MAXTRANSFERSIZE	0x0fff /*the max length is denoted by 0-11bit*/
#define MAXTRANSFERSIZE		DMAC_MAXTRANSFERSIZE
#define DMAC_CXDISABLE		0x00
#define DMAC_CXENABLE		0x01

/*the means the bit in the channel control register*/
#define DMAC_CxCONFIG_M2M			0xc3344080

/*#define DMAC_CxCONFIG_M2M  0x4001*/
#define DMAC_CHANNEL_ENABLE			1
#define DMAC_CHANNEL_DISABLE			0xfffffffe

/*default the config and sync regsiter for DMAC controller*/
/*M1,M2 little endian, enable DMAC*/
#define DMAC_CONFIG_VAL				0x01

/*enable the sync logic for the 16 peripheral*/
#define DMAC_SYNC_VAL				0x0

/*definition for the return value*/
#define DMAC_ERROR_BASE				100
#define DMAC_CHANNEL_INVALID			(DMAC_ERROR_BASE + 1)

#define DMAC_TRXFERSIZE_INVALID			(DMAC_ERROR_BASE + 2)
#define DMAC_SOURCE_ADDRESS_INVALID		(DMAC_ERROR_BASE + 3)
#define DMAC_DESTINATION_ADDRESS_INVALID	(DMAC_ERROR_BASE + 4)
#define DMAC_MEMORY_ADDRESS_INVALID		(DMAC_ERROR_BASE + 5)
#define DMAC_PERIPHERAL_ID_INVALID		(DMAC_ERROR_BASE + 6)
#define DMAC_DIRECTION_ERROR			(DMAC_ERROR_BASE + 7)
#define DMAC_TRXFER_ERROR			(DMAC_ERROR_BASE + 8)
#define DMAC_LLIHEAD_ERROR			(DMAC_ERROR_BASE + 9)
#define DMAC_SWIDTH_ERROR			(DMAC_ERROR_BASE + 0xa)
#define DMAC_LLI_ADDRESS_INVALID		(DMAC_ERROR_BASE + 0xb)
#define DMAC_TRANS_CONTROL_INVALID		(DMAC_ERROR_BASE + 0xc)
#define DMAC_MEMORY_ALLOCATE_ERROR		(DMAC_ERROR_BASE + 0xd)
#define DMAC_NOT_FINISHED			(DMAC_ERROR_BASE + 0xe)

#define DMAC_TIMEOUT				(DMAC_ERROR_BASE + 0xf)
#define DMAC_CHN_SUCCESS			(DMAC_ERROR_BASE + 0x10)
#define DMAC_CHN_CONFIG_ERROR			(DMAC_ERROR_BASE + 0x11)
#define DMAC_CHN_DATA_ERROR			(DMAC_ERROR_BASE + 0x12)
#define DMAC_CHN_TIMEOUT			(DMAC_ERROR_BASE + 0x13)
#define DMAC_CHN_ALLOCAT			(DMAC_ERROR_BASE + 0x14)
#define DMAC_CHN_VACANCY			(DMAC_ERROR_BASE + 0x15)
#define DMAC_CONFIGURATIONx_ACTIVE_NOT		0
#define DMAC_MAX_PERIPHERALS			16
#define DMAC_MAX_CHANNELS			CONFIG_HIDMAC_CHN_NUM
#define MEM_MAX_NUM				3
#define DEST_IS_KERNEL				(1 << 17)
#define SRC_IS_KERNEL				(1 << 5)

/*DMAC peripheral structure*/
typedef struct dmac_peripheral {
	unsigned int peri_id;	/* peripherial ID*/
	void *pperi_addr;	/*peripheral data register address*/
	unsigned int transfer_ctrl;	/*default channel control word*/
	unsigned int transfer_cfg;	/*default channel configuration word*/
} dmac_peripheral;

typedef struct mem_addr {
	unsigned int addr_base;
	unsigned int size;
} mem_addr;

extern int g_channel_status[DMAC_MAX_CHANNELS];
#endif /* End of #ifndef __HI_INC_ECSDMACC_H__ */
