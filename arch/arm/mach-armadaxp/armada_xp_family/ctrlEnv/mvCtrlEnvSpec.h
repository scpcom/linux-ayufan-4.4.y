 
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
#define MV_RUNIT_PMU_REGS_OFFSET		(0x1C000)
#define MV_MPP_REGS_OFFSET			(0x18000)
#define MV_GPP_REGS_OFFSET(unit)		(0x18100 + ((unit) * 0x40))
#define MV_MISC_REGS_OFFSET			(0x18200)
#define MV_CLK_CMPLX_REGS_OFFSET	(0x18700)
#define MV_MBUS_REGS_OFFSET			(0x20000)
#define MV_COHERENCY_FABRIC_OFFSET		(0x20200)
#define MV_CIB_CTRL_STATUS_OFFSET		(0x20280)
#define MV_CNTMR_REGS_OFFSET			(0x20300)
#define MV_CPUIF_LOCAL_REGS_OFFSET		(0x21000)
#define MV_CPUIF_REGS_OFFSET(cpu)		(0x21800 + (cpu) * 0x100)
#define MV_PMU_NFABRIC_UNIT_SERV_OFFSET		(0x22000)
#define MV_CPU_PMU_UNIT_SERV_OFFSET(cpu)	(0x22100 + (cpu) * 0x100)
#define MV_CPU_HW_SEM_OFFSET			(0x20500)

#if defined(MV_ETH_LEGACY)
	#define MV_ETH_BASE_ADDR		(0x72000)
#else
	#define MV_ETH_BASE_ADDR		(0x70000)
#endif
#define MV_ETH_REGS_OFFSET(port)		(MV_ETH_BASE_ADDR - ((port) / 2) * 0x40000 + ((port) % 2) * 0x4000)
#define MV_PEX_IF_REGS_OFFSET(pexIf)		(pexIf < 8 ? (0x40000 + ((pexIf) / 4) * 0x40000 + ((pexIf) % 4) * 0x4000) \
											 : (0x42000 + ((pexIf) % 8) * 0x40000))
#define MV_USB_REGS_OFFSET(dev)       		(0x50000 + (dev * 0x1000))
#define MV_XOR_REGS_OFFSET(unit)		(unit ? 0xF0900 : 0x60900)
#if defined(MV_INCLUDE_IDMA)
#define MV_IDMA_REGS_OFFSET			(0x60800)
#endif
#define MV_CESA_TDMA_REGS_OFFSET(chanNum)	(0x90000 + (chanNum * 0x2000))
#define MV_CESA_REGS_OFFSET(chanNum)		(0x9D000 + (chanNum * 0x2000))
#define MV_SATA_REGS_OFFSET			(0xA0000)
#define MV_COMM_UNIT_REGS_OFFSET		(0xB0000)
#define MV_NFC_REGS_OFFSET			(0xD0000)
#define MV_BM_REGS_OFFSET			(0xC0000)
#define MV_PNC_REGS_OFFSET			(0xC8000)
#define MV_SDMMC_REGS_OFFSET			(0xD4000)

#ifdef CONFIG_ARMADA_XP_ERRATA_SMI_1
	#define MV_ETH_SMI_PORT   1
#else
    #define MV_ETH_SMI_PORT   0
#endif

#define MV_SERDES_NUM_TO_PEX_NUM(sernum)	((sernum < 8) ? (sernum) : (8 + (sernum/12)))
 
#define AVS_CONTROL2_REG			0x20868
#define AVS_LOW_VDD_LIMIT			0x20860

#define INTER_REGS_SIZE				_1M

#define TWSI_CPU_MAIN_INT_CAUSE_REG(cpu)	CPU_MAIN_INT_CAUSE_REG(1, (cpu))
#define TWSI0_CPU_MAIN_INT_BIT(ch)		((ch) + 3)
#define TWSI_SPEED				100000

#define MV_GPP_MAX_PINS				68
#define MV_GPP_MAX_GROUP    			3  
#define MV_CNTMR_MAX_COUNTER 		8  

#define MV_UART_MAX_CHAN			4

#define MV_XOR_MAX_UNIT				2  
#define MV_XOR_MAX_CHAN         		4  
#define MV_XOR_MAX_CHAN_PER_UNIT		2  

#if defined(MV_INCLUDE_IDMA)
#define MV_IDMA_MAX_UNIT			1  
#define MV_IDMA_MAX_CHAN			4  
#endif

#define MV_SATA_MAX_CHAN			2

#define MV_MPP_MAX_GROUP			9

#define MV_DRAM_MAX_CS				4
#define MV_SPI_MAX_CS				8
 
#ifdef MV_INCLUDE_PCI
 #define MV_PCI_MAX_IF				1
 #define MV_PCI_START_IF			0
 #define PCI_HOST_BUS_NUM(pciIf)               (pciIf)
 #define PCI_HOST_DEV_NUM(pciIf)               0
#else
 #define MV_PCI_MAX_IF				0
 #define MV_PCI_START_IF			0
#endif

#define MV_PEX_MAX_IF				10
#define MV_PEX_MAX_UNIT				4
#ifdef MV_INCLUDE_PEX
#define MV_INCLUDE_PEX0
#define MV_DISABLE_PEX_DEVICE_BAR

#define MV_PEX_START_IF				MV_PCI_MAX_IF
 #define PEX_HOST_BUS_NUM(pciIf)               (pciIf)
 #define PEX_HOST_DEV_NUM(pciIf)               0
#else
 #undef MV_INCLUDE_PEX0
#endif

#define PCI_IO(pciIf)				(PEX0_IO + 2 * (pciIf))
#define PCI_MEM(pciIf, memNum)			(PEX0_MEM0 + 2 * (pciIf))
 
#define MV_IDMA_MAX_CHAN			4
#define ARMADA_XP_MAX_USB_PORTS			3
#define ARMADA_XP_NAND				1
#define ARMADA_XP_SDIO				1
#define ARMADA_XP_MAX_TDM_PORTS			32
#define ARMADA_XP_TDM				1
#define MV_DEVICE_MAX_CS      			4

#ifndef MV_USB_MAX_PORTS
#define MV_USB_MAX_PORTS (ARMADA_XP_MAX_USB_PORTS)
#endif

#define MV_CESA_VERSION				3  
#define MV_CESA_SRAM_SIZE               	(2 * 1024)

#define MV_ETH_VERSION 				4  
#define MV_NETA_VERSION				1  
#define MV_ETH_MAX_PORTS			4
#define MV_ETH_MAX_RXQ              		8
#define MV_ETH_MAX_TXQ              		8
#define MV_ETH_TX_CSUM_MAX_SIZE 		9800
#define MV_PNC_TCAM_LINES			1024	 

#define MV_ETH_GMAC_NEW
 
#define MV_ETH_WRR_NEW
 
#define MV_ETH_LEGACY_PARSER_IPV6
 
#define MV_ETH_PNC_NEW
 
#define MV_ETH_PNC_LB

#define MV_78130_ETH_MAX_PORT			3
#define MV_78460_ETH_MAX_PORT			4

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

typedef enum {
	TDM_UNIT_32CH
} MV_TDM_UNIT_TYPE;

typedef enum _mvUnitId {
	DRAM_UNIT_ID,
	PEX_UNIT_ID,
	ETH_GIG_UNIT_ID,
	USB_UNIT_ID,
	IDMA_UNIT_ID,
	XOR_UNIT_ID,
	SATA_UNIT_ID,
	TDM_32CH_UNIT_ID,
	UART_UNIT_ID,
	CESA_UNIT_ID,
	SPI_UNIT_ID,
	SDIO_UNIT_ID,
	BM_UNIT_ID,
	PNC_UNIT_ID,
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
	PEX2_MEM,	 
	PEX2_IO,	 
	PEX3_MEM,	 
	PEX3_IO,	 
	PEX4_MEM,	 
	PEX4_IO,	 
	PEX5_MEM,	 
	PEX5_IO,	 
	PEX6_MEM,	 
	PEX6_IO,	 
	PEX7_MEM,	 
	PEX7_IO,	 
	PEX8_MEM,	 
	PEX8_IO,	 
	PEX9_MEM,	 
	PEX9_IO,	 
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
	CRYPT1_ENG,	 
	PNC_BM,		 
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

#ifdef CONFIG_MACH_ARMADA_XP_FPGA
 #define MAIN_BOOT_ATTR		0x2F	 
 #define SEC_BOOT_ATTR		0x1D	 
#else
 #define MAIN_BOOT_ATTR		0x1D	 
 #define SEC_BOOT_ATTR		0x2F	 
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
	{0xE8, PEX0_2_TARGET_ID },  	\
	{0xE0, PEX0_2_TARGET_ID },  	\
	{0xD8, PEX0_2_TARGET_ID },  	\
	{0xD0, PEX0_2_TARGET_ID },  	\
	{0xB8, PEX0_2_TARGET_ID },  	\
	{0xB0, PEX0_2_TARGET_ID },  	\
	{0x78, PEX0_2_TARGET_ID },  	\
	{0x70, PEX0_2_TARGET_ID },  	\
	{0xE8, PEX1_3_TARGET_ID },  	\
	{0xE0, PEX1_3_TARGET_ID },  	\
	{0xD8, PEX1_3_TARGET_ID },  	\
	{0xD0, PEX1_3_TARGET_ID },  	\
	{0xB8, PEX1_3_TARGET_ID },  	\
	{0xB0, PEX1_3_TARGET_ID },  	\
	{0x78, PEX1_3_TARGET_ID },  	\
	{0x70, PEX1_3_TARGET_ID },  	\
	{0xF8, PEX0_2_TARGET_ID },  	\
	{0xF0, PEX0_2_TARGET_ID },  	\
	{0xF8, PEX1_3_TARGET_ID },  	\
	{0xF0, PEX1_3_TARGET_ID },  	\
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
	{MAIN_BOOT_ATTR, DEV_TARGET_ID    },  	\
	{SEC_BOOT_ATTR, DEV_TARGET_ID    },  	\
	{0x2D, DEV_TARGET_ID    },  	\
	{0x01, CRYPT_TARGET_ID  },  	\
	{0x05, CRYPT_TARGET_ID  },       \
	{0x00, PNC_BM_TARGET_ID },  		\
}

#define CESA_TARGET_NAME_DEF	("CRYPT_ENG0", "CRYPT_ENG1")
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
	"PEX2_MEM",	 		\
	"PEX2_IO",	 		\
	"PEX3_MEM",	 		\
	"PEX3_IO",	 		\
	"PEX4_MEM",	 		\
	"PEX4_IO",	 		\
	"PEX5_MEM",	 		\
	"PEX5_IO",	 		\
	"PEX6_MEM",	 		\
	"PEX6_IO",	 		\
	"PEX7_MEM",	 		\
	"PEX7_IO",	 		\
	"PEX8_MEM",	 		\
	"PEX8_IO",	 		\
	"PEX9_MEM",	 		\
	"PEX9_IO",	 		\
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
	"CRYPT1_ENG",	 	\
	"CRYPT2_ENG",	 	\
	"PNC_BM"	 		\
}

#endif  

#ifdef __cplusplus
}
#endif  

#endif  
