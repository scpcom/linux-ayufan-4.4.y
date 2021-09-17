#ifndef __HI_DMAC_HI3521A_H__
#define __HI_DMAC_HI3521A_H__

#define DDRAM_ADRS		DDR_MEM_BASE
#define DDRAM_SIZE		0x7FFFFFFF	/* 2GB DDR. */

#define FLASH_BASE		0x10000000
#define FLASH_SIZE		0x04000000	/* (32MB) */

#define DMAC_BASE_REG		CONFIG_HI_DMAC_IO_BASE
#define DMAC_INTTCCLEAR		IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x08)

#define DMAC_INTSTATUS		IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x00)
#define DMAC_INTTCSTATUS	IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x04)
#define DMAC_INTERRORSTATUS	IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x0C)

#define DMAC_INTERRCLR		IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x10)
#define DMAC_RAWINTTCSTATUS	IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x14)
#define DMAC_RAWINTERRORSTATUS	IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x18)
#define DMAC_ENBLDCHNS		IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x1C)
#define DMAC_CONFIG		IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x30)
#define DMAC_SYNC		IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x34)

#define DMAC_MAXTRANSFERSIZE	0x0fff /*the max length is denoted by 0-11bit*/
#define MAXTRANSFERSIZE		DMAC_MAXTRANSFERSIZE
#define DMAC_CxDISABLE		0x00
#define DMAC_CxENABLE		0x01

/*the definition for DMAC channel register*/
#define DMAC_CxBASE(i)		IO_DMAC_ADDRESS(DMAC_BASE_REG + 0x100 \
						+ i * 0x20)
#define DMAC_CxSRCADDR(i)	DMAC_CxBASE(i)
#define DMAC_CxDESTADDR(i)	(DMAC_CxBASE(i) + 0x04)
#define DMAC_CxLLI(i)		(DMAC_CxBASE(i) + 0x08)
#define DMAC_CxCONTROL(i)	(DMAC_CxBASE(i) + 0x0C)
#define DMAC_CxCONFIG(i)	(DMAC_CxBASE(i) + 0x10)

/*the means the bit in the channel control register*/
#define DMAC_CxCONTROL_M2M	0x9d480000 /* Dwidth=32,burst size=4 */
#define DMAC_CxCONTROL_LLIM2M	0x0f480000 /* Dwidth=32,burst size=1 */
#define DMAC_CxLLI_LM		0x01

#define DMAC_CxCONFIG_M2M	0xc000
#define DMAC_CxCONFIG_LLIM2M	0xc000

/*#define DMAC_CxCONFIG_M2M 0x4001*/
#define DMAC_CHANNEL_ENABLE	1
#define DMAC_CHANNEL_DISABLE	0xfffffffe

#define DMAC_CxCONTROL_P2M	0x89409000
#define DMAC_CxCONFIG_P2M	0xd000

#define DMAC_CxCONTROL_M2P	0x86089000
#define DMAC_CxCONFIG_M2P	0xc800

#define DMAC_CxCONFIG_SIO_P2M	0x0000d000
#define DMAC_CxCONFIG_SIO_M2P	0x0000c800

/*default the config and sync regsiter for DMAC controller*/
/*M1,M2 little endian, enable DMAC*/
#define DMAC_CONFIG_VAL		0x01
/*enable the sync logic for the 16 peripheral*/
#define DMAC_SYNC_VAL		0x0

#define DMAC_MAX_PERIPHERALS	16
#define MEM_MAX_NUM		2
#define CHANNEL_NUM		CONFIG_HI_DMAC_CHANNEL_NUM
#define DMAC_MAX_CHANNELS	CHANNEL_NUM

/* Uart data register address */
#define UART0_DATA_REG		(REG_BASE_UART0 + REG_UART_DATA)
#define UART1_DATA_REG		(REG_BASE_UART1 + REG_UART_DATA)
#define UART2_DATA_REG		(REG_BASE_UART2 + REG_UART_DATA)

/* SPI data register address */
#define SSP_DATA_REG		(SSP_REG_BASE + REG_SSP_DATA)

/* I2c data register address */
#define I2C_DATA_REG		(I2C_REG_BASE + REG_I2C_DATA)

/*the transfer control and configuration value for different peripheral*/

extern int g_channel_status[CHANNEL_NUM];
extern dmac_peripheral g_peripheral[DMAC_MAX_PERIPHERALS];

#endif /* End of __HI_DMAC_HI3521A_H__ */
