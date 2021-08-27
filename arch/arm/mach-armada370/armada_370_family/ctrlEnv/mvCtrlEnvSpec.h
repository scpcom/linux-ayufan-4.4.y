#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvCtrlEnvSpech
#define __INCmvCtrlEnvSpech

#include "mvDeviceId.h"
#include "mvSysHwConfig.h"

#include "ctrlEnv/sys/mvCpuIfRegs.h"
#ifdef __cplusplus
extern "C" {
#endif  

#define MV_ARM_SOC
#define SOC_NAME_PREFIX				"MV88F"


#define MV_DRAM_REGS_OFFSET			(0x0)
#define MV_AURORA_L2_REGS_OFFSET		(0x8000)
#define MV_RTC_REGS_OFFSET			(0x10300)
#ifdef CONFIG_SYNO_ARMADA
#define MV_RTC_EXTERNAL_ALARM_OFFSET			(0x10320)
#endif
#define MV_DEV_BUS_REGS_OFFSET			(0x10400)
#define MV_SPI_REGS_OFFSET(unit)		(0x10600 + (unit * 0x80))
#define MV_TWSI_SLAVE_REGS_OFFSET(chanNum)	(0x11000 + (chanNum * 0x100))
#define MV_UART_REGS_OFFSET(chanNum)		(0x12000 + (chanNum * 0x100))
#define MV_MPP_REGS_OFFSET			(0x18000)
#define MV_GPP_REGS_OFFSET(unit)		(0x18100 + ((unit) * 0x40))
#define MV_MISC_REGS_OFFSET			(0x18200)
#define MV_RUNIT_PMU_REGS_OFFSET		(0x1C000)
#define MV_MBUS_REGS_OFFSET			(0x20000)
#define MV_COHERENCY_FABRIC_OFFSET		(0x20200)
#define MV_CIB_CTRL_STATUS_OFFSET		(0x20280)
#define MV_CNTMR_REGS_OFFSET			(0x20300)
#define MV_CPUIF_LOCAL_REGS_OFFSET		(0x21800)
#define MV_AVS_REGS_OFFSET				(0x20860)
#define MV_CPUIF_REGS_OFFSET(x)			(0x21000)
#define MV_PMU_NFABRIC_UNIT_SERV_OFFSET		(0x22000)
#define MV_CPU_PMU_UNIT_SERV_OFFSET(cpu)	(0x22100 + (cpu) * 0x100)
#define MV_AUDIO_REGS_OFFSET(unit)		(0x30000)
#define MV_CPU_HW_SEM_OFFSET			(0x20500)

#if defined(MV_ETH_LEGACY)
	#define MV_ETH_BASE_ADDR		(0x72000)
#else
	#define MV_ETH_BASE_ADDR		(0x70000)
#endif
#define MV_ETH_SGMII_PHY_REGS_OFFSET(port)	(0x72000 + (port) * 0x4000)
#define MV_ETH_REGS_OFFSET(port)		(MV_ETH_BASE_ADDR + (port) * 0x4000)
#define MV_PEX_IF_REGS_OFFSET(pexIf)		(0x40000 + (pexIf * 0x40000))
#define MV_USB_REGS_OFFSET(dev)       		(0x50000 + (dev * 0x1000))
#define MV_XOR_REGS_OFFSET(unit)			(0x60800 + (unit * 0x100))
#define MV_CESA_TDMA_REGS_OFFSET(chanNum)	(0x90000 + (chanNum * 0x2000))
#define MV_CESA_REGS_OFFSET(chanNum)		(0x9D000 + (chanNum * 0x2000))
#define MV_SATA_REGS_OFFSET			(0xA0000)
#define MV_TDM_REGS_OFFSET			(0xB0000)
#define MV_NFC_REGS_OFFSET			(0xD0000)
#define MV_SDMMC_REGS_OFFSET			(0xD4000)

#define MV_ETH_SMI_PORT				0

#define MV_SERDES_NUM_TO_PEX_NUM(sernum)	(sernum)
 
#define INTER_REGS_SIZE				_1M

#define TWSI_CPU_MAIN_INT_CAUSE_REG(cpu)	CPU_MAIN_INT_CAUSE_REG(1)
#define TWSI_SPEED				100000

#define MV_GPP_MAX_PINS				65
#define MV_GPP_MAX_GROUP    		3  
#define MV_CNTMR_MAX_COUNTER		8
 
#define MV_UART_MAX_CHAN			2

#define MV_XOR_MAX_UNIT				2  
#define MV_XOR_MAX_CHAN         		4  
#define MV_XOR_MAX_CHAN_PER_UNIT		2  

#define MV_SATA_MAX_CHAN			2
#define MV_SATA_MV6W11_CHAN			0

#define MV_AUDIO_MAX_UNITS			1

#define MV_MPP_MAX_GROUP			9
#define MV_SERDES_MAX_LANES			4

#define MV_DRAM_MAX_CS				4
#define MV_SPI_MAX_CS				8
 
#ifdef MV_INCLUDE_PCI
 #define MV_PCI_MAX_IF				1
 #define MV_PCI_START_IF			0
 #define PCI_HOST_BUS_NUM(pciIf)		(pciIf)
 #define PCI_HOST_DEV_NUM(pciIf)		0
#else
 #define MV_PCI_MAX_IF				0
 #define MV_PCI_START_IF			0
#endif

#ifdef MV_INCLUDE_PEX
#define MV_INCLUDE_PEX0
#define MV_DISABLE_PEX_DEVICE_BAR

#define MV_PEX_MAX_IF				2
#define MV_PEX_START_IF				MV_PCI_MAX_IF
#define MV_PEX_MAX_UNIT				2
 #define PEX_HOST_BUS_NUM(pciIf)		(pciIf)
 #define PEX_HOST_DEV_NUM(pciIf)		0
#else
 #undef MV_INCLUDE_PEX0
#endif

#define PCI_IO(pciIf)				(PEX0_IO + 2 * (pciIf))
#define PCI_MEM(pciIf, memNum)			(PEX0_MEM0 + 2 * (pciIf))
 
#define MV_IDMA_MAX_CHAN			4
#define MV_MAX_USB_PORTS			2
#define ARMADA_370_NAND				1
#define ARMADA_370_SDIO				1
#define ARMADA_370_MAX_TDM_PORTS		2
#define MV_DEVICE_MAX_CS      			4
#define ARMADA_370_TDM				1

#ifndef MV_USB_MAX_PORTS
#define MV_USB_MAX_PORTS			MV_MAX_USB_PORTS
#endif

#define MV_CESA_VERSION				3  
#define MV_CESA_SRAM_SIZE			(2 * 1024)


#define MV_ETH_VERSION 				4  
#define MV_NETA_VERSION				1  
#define MV_ETH_MAX_PORTS			2
#define MV_ETH_MAX_RXQ				8
#define MV_ETH_MAX_TXQ				8
#if defined(CONFIG_SYNO_ARMADA)
#define MV_ETH_TX_CSUM_MAX_SIZE 		1800
#else
#define MV_ETH_TX_CSUM_MAX_SIZE 		2048
#endif  

#define MV_ETH_LEGACY_PARSER_IPV6

#define MV_ETH_GMAC_NEW
 
#define MV_ETH_WRR_NEW

#define MV_USB_VERSION  			1

#define MV_SPI_VERSION				2

#define MV_INCLUDE_SDRAM_CS0
#define MV_INCLUDE_SDRAM_CS1
#define MV_INCLUDE_SDRAM_CS2
#define MV_INCLUDE_SDRAM_CS3

#define MV_INCLUDE_DEVICE_CS0
#define MV_INCLUDE_DEVICE_CS1
#define MV_INCLUDE_DEVICE_CS2
#define MV_INCLUDE_DEVICE_CS3

#ifndef MV_ASMLANGUAGE

#define TBL_UNUSED	0	 

#define MPP_GROUP_1_TYPE {\
	 	\
	{{0x00000000, 0x00000000}, {0x00000000, 0x00000000},  		\
	 {0x00000000, 0x00000000}, {0x00000000, 0x00000000} },					\
	{{0xFFF00000, 0x44400000}, {0xFFFFFF0F, 0x44446404},  			\
	 {0x0000000F, 0x00000004}, {0x00000000, 0x00000000} },					\
	{{0xFFF00000, 0x55500000}, {0x00FF0FFF, 0x00550555},  	\
	 {0x00000000, 0x00000000}, {0x00000000, 0x00000000} },					\
	{{0xFFF00000, 0x11100000}, {0xFFFFFFFF, 0x11111111},  	 	\
	 {0xFFFFF00F, 0x11111001}, {0xFFFFFFFF, 0x41111111} },				 	\
	{{0x00000000, 0x00000000}, {0xFFFFF0F0, 0x33333030},  			\
	 {0x00000000, 0x00000000}, {0x00000000, 0x00000000} },					\
	{{0xFFF00000, 0x11100000}, {0xFFFFFFFF, 0x11111111},  		\
	 {0x0000000F, 0x00000001}, {0x00000000, 0x00000000} },					\
	{{0x00000000, 0x00000000}, {0x00000000, 0x00000000},  		\
	 {0xFFFFF000, 0x22222000}, {0x0FFFFFFF, 0x02222222} },					\
}

#define MPP_GROUP_2_TYPE {\
	 	\
	{{0x00000000, 0x00000000}, {0x00000000, 0x00000000},  	\
	 {0x00000000, 0x00000000}, {0x00000000, 0x00000000} },					\
	{{0xFFFFFFF0, 0x11111110}, {0x0FFFFFFF, 0x01111111},  	\
	 {0xFFFFFFF0, 0x33344440}, {0x0F000FFF, 0x03000336} },					\
	{{0x00000000, 0x00000000}, {0xF0000000, 0x50000000},  	\
	 {0x00000FF0, 0x00000550}, {0x0FFFFFF0, 0x05555550} },					\
	{{0x00000000, 0x00000000}, {0x00000000, 0x00000000},  		 	\
	 {0x00FFFFFF, 0x00233333}, {0x00000000, 0x00000000} },				 	\
	{{0x00000000, 0x00000000}, {0x00000000, 0x00000000}, 					\
	 {0x00000000, 0x00000000}, {0x00000000, 0x00000000} },					\
	{{0x00000000, 0x00000000}, {0x00000000, 0x00000000}, 					\
	 {0x00000000, 0x00000000}, {0x00000000, 0x00000000} },					\
	{{0x00000000, 0x00000000}, {0x00000000, 0x00000000}, 					\
	 {0x00000000, 0x00000000}, {0x00000000, 0x00000000} },					\
}

typedef enum {
	TDM_UNIT_2CH
} MV_TDM_UNIT_TYPE;

typedef enum _mvUnitId {
	DRAM_UNIT_ID,
	PEX_UNIT_ID,
	ETH_GIG_UNIT_ID,
	USB_UNIT_ID,
	XOR_UNIT_ID,
	SATA_UNIT_ID,
	TDM_2CH_UNIT_ID,
	UART_UNIT_ID,
	CESA_UNIT_ID,
	SPI_UNIT_ID,
	SDIO_UNIT_ID,
	AUDIO_UNIT_ID,
	MAX_UNITS_ID
} MV_UNIT_ID;

typedef enum _mvDevice {
#if defined(MV_INCLUDE_DEVICE_CS0)
	DEV_CS0 = 0,     
#endif
#if defined(MV_INCLUDE_DEVICE_CS1)
	DEV_CS1 = 1,         
#endif
#if defined(MV_INCLUDE_DEVICE_CS2)
	DEV_CS2 = 2,         
#endif
#if defined(MV_INCLUDE_DEVICE_CS3)
	DEV_CS3 = 3,         
#endif
	BOOT_CS,         
	MV_DEV_MAX_CS = MV_DEVICE_MAX_CS
} MV_DEVICE;

typedef enum _mvTarget {
	TBL_TERM = -1, 	 
	SDRAM_CS0,	 
	SDRAM_CS1,	 
	SDRAM_CS2,	 
	SDRAM_CS3,	 
	DEVICE_CS0,	 
	DEVICE_CS1,	 
	DEVICE_CS2,	 
	DEVICE_CS3,	 
	PEX0_MEM,	 
	PEX0_IO,	 
	PEX1_MEM,	 
	PEX1_IO,	 
	INTER_REGS,	 
	DMA_UART,	 
	SPI_CS0,	 
	SPI_CS1,	 
	SPI_CS2,	 
	SPI_CS3,	 
	SPI_CS4,	 
	SPI_CS5,	 
	SPI_CS6,	 
	SPI_CS7,	 
	BOOT_ROM_CS,  
	DEV_BOOCS,	 
	PMU_SCRATCHPAD,	 
	CRYPT0_ENG,	 
	MAX_TARGETS
} MV_TARGET;

#ifdef AURORA_IO_CACHE_COHERENCY
#define DRAM_CS0_ATTR		0x1E
#define DRAM_CS1_ATTR		0x1D
#define DRAM_CS2_ATTR		0x1B
#define DRAM_CS3_ATTR		0x17
#else
#define DRAM_CS0_ATTR		0x0E
#define DRAM_CS1_ATTR		0x0D
#define DRAM_CS2_ATTR		0x0B
#define DRAM_CS3_ATTR		0x07
#endif

#define TARGETS_DEF_ARRAY	{			\
	{DRAM_CS0_ATTR, DRAM_TARGET_ID   },  	\
	{DRAM_CS1_ATTR, DRAM_TARGET_ID   },  	\
	{DRAM_CS2_ATTR, DRAM_TARGET_ID   },  	\
	{DRAM_CS3_ATTR, DRAM_TARGET_ID   },  	\
	{0x3E, DEV_TARGET_ID    },  	\
	{0x3D, DEV_TARGET_ID    },  	\
	{0x3B, DEV_TARGET_ID    },  	\
	{0x37, DEV_TARGET_ID    },  	\
	{0x18, PEX0_TARGET_ID },  	\
	{0x10, PEX0_TARGET_ID },  	\
	{0x28, PEX1_TARGET_ID },  	\
	{0x20, PEX1_TARGET_ID },  	\
	{0xFF, 0xFF             },  	\
	{0x01, DEV_TARGET_ID    },  	\
	{0x1E, DEV_TARGET_ID    },  	\
	{0x5E, DEV_TARGET_ID    },  	\
	{0x9E, DEV_TARGET_ID    },  	\
	{0xDE, DEV_TARGET_ID    },  	\
	{0x1F, DEV_TARGET_ID    },  	\
	{0x5F, DEV_TARGET_ID    },  	\
	{0x9F, DEV_TARGET_ID    },  	\
	{0xDF, DEV_TARGET_ID    },  	\
	{0x1D, DEV_TARGET_ID    },  	\
	{0x2F, DEV_TARGET_ID    },  	\
	{0x2D, DEV_TARGET_ID    },  	\
	{0x01, CRYPT_TARGET_ID  },  	\
}

#define TARGETS_NAME_ARRAY	{		\
	"SDRAM_CS0",     		\
	"SDRAM_CS1",     		\
	"SDRAM_CS2",     		\
	"SDRAM_CS3",     		\
	"DEVICE_CS0",	 	\
	"DEVICE_CS1",	 	\
	"DEVICE_CS2",	 	\
	"DEVICE_CS3",	 	\
	"PEX0_MEM",	 		\
	"PEX0_IO",	 		\
	"PEX1_MEM",	 		\
	"PEX1_IO",	 		\
	"INTER_REGS",	 	\
	"DMA_UART",	 		\
	"SPI_CS0",	 		\
	"SPI_CS1",	 		\
	"SPI_CS2",	 		\
	"SPI_CS3",	 		\
	"SPI_CS4",	 		\
	"SPI_CS5",	 		\
	"SPI_CS6",	 		\
	"SPI_CS7",	 		\
	"BOOT_ROM_CS",	 	\
	"DEV_BOOTCS",	 		\
	"PMU_SCRATCHPAD", 	\
	"CRYPT0_ENG",	 	\
}

#endif  

#ifdef __cplusplus
}
#endif  

#endif  
