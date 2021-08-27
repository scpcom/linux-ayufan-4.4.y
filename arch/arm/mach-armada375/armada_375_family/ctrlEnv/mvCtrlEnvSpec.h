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
#define SOC_NAME_PREFIX                         "MV88F"


#define MV_DRAM_REGS_OFFSET                     (0x0)
#define MV_AURORA_L2_REGS_OFFSET                (0x8000)
#define MV_RTC_REGS_OFFSET                      (0x10300)
#ifdef CONFIG_SYNO_ARMADA_V2
#define MV_RTC_EXTERNAL_ALARM_OFFSET			(0x10320)
#endif
#define MV_DEV_BUS_REGS_OFFSET                  (0x10400)

#define MV_SPI_REGS_OFFSET(unit)                (0x10600 + (unit * 0x80))
#define MV_TWSI_SLAVE_REGS_OFFSET(chanNum)      (0x11000 + (chanNum * 0x100))
#define MV_UART_REGS_OFFSET(chanNum)            (0x12000 + (chanNum * 0x100))
#define MV_MPP_REGS_OFFSET                      (0x18000)
#define MV_GPP_REGS_OFFSET(unit)                (0x18100 + ((unit) * 0x40))
#define MV_MISC_REGS_OFFSET                     (0x18200)
#define MV_COMMON_PHY_REGS_OFFSET               (0x18300)
#define MV_IP_CONFIG_REGS_OFFSET                (0x18400)
#define MV_CLK_CMPLX_REGS_OFFSET        	(0x18700)
#define MV_ETH_COMPLEX_OFFSET			(0x18900)
#define MV_MBUS_REGS_OFFSET                     (0x20000)
#define CPU_GLOBAL_BASE                         (MV_MBUS_REGS_OFFSET)
#define MV_COHERENCY_FABRIC_OFFSET              (0x20200)  
#define MV_CIB_CTRL_STATUS_OFFSET               (0x20280)
#define MV_CNTMR_REGS_OFFSET                    (0x20300)

#define MV_CPUIF_LOCAL_REGS_OFFSET              (0x21000)
#define MV_CPUIF_REGS_OFFSET(cpu)               (0x21800 + (cpu) * 0x100)
#define MV_CPU_HW_SEM_OFFSET                    (0x20500)

#define MV_PP2_REG_BASE                         (0xF0000)
#define MV_ETH_BASE_ADDR                        (0xC0000)
#define LMS_REG_BASE                            (MV_ETH_BASE_ADDR)
#define MIB_COUNTERS_REG_BASE                   (MV_ETH_BASE_ADDR + 0x1000)
#define GOP_MNG_REG_BASE                        (MV_ETH_BASE_ADDR + 0x3000)
#define GOP_REG_BASE(port)                      (MV_ETH_BASE_ADDR + 0x4000 + ((port) / 2) * 0x3000 + ((port) % 2) * 0x1000)
#define MV_PON_REGS_OFFSET                      (MV_ETH_BASE_ADDR + 0x8000)

#define MV_PP2_PON_EXIST
#define MV_ETH_MAX_TCONT                        16
#define MV_ETH_RXQ_TOTAL_NUM                    32

#define MV_PEX_IF_REGS_OFFSET(pexIf)            (pexIf < 8 ? (0x40000 + ((pexIf) / 4) * 0x40000 + ((pexIf) % 4) * 0x4000) \
						 : (0x42000 + ((pexIf) % 8) * 0x40000))
#define MV_USB_REGS_OFFSET(dev)                 (0x50000 + (0x4000 * dev))
#define MV_USB3_REGS_OFFSET(dev)                ((mvCtrlRevGet() <= MV_88F6720_Z3_ID) ? 0x5ff80 : 0x5b880)
#define MV_XOR_REGS_OFFSET(unit)                (0x60800)
#define MV_CESA_TDMA_REGS_OFFSET(chanNum)       (0x90000 + (chanNum * 0x2000))
#define MV_CESA_REGS_OFFSET(chanNum)            (0x9D000 + (chanNum * 0x2000))
#define MV_SATA_REGS_OFFSET                     (0xA0000)
#define MV_COMM_UNIT_REGS_OFFSET                (0xB0000)
#define MV_NFC_REGS_OFFSET                      (0xD0000)
#define MV_SDMMC_REGS_OFFSET                    (0xD4000)

#define MV_ETH_SMI_PORT   0

#define MV_USB3_WIN_BASE(unitId)		MV_USB3_REGS_OFFSET(unitId)

#define INTER_REGS_SIZE                         _1M

#define TWSI_CPU_MAIN_INT_CAUSE_REG(cpu)        CPU_MAIN_INT_CAUSE_REG(1, (cpu))
#define TWSI0_CPU_MAIN_INT_BIT(ch)              ((ch) + 3)
#define TWSI_SPEED                              100000

#define MV_GPP_MAX_PINS                         68
#define MV_GPP_MAX_GROUP                        3        
#define MV_CNTMR_MAX_COUNTER            17               

#define MV_IO_EXP_MAX_REGS			3

#define MV_UART_MAX_CHAN                        4

#define MV_XOR_MAX_UNIT                         2        
#define MV_XOR_MAX_CHAN                         4        
#define MV_XOR_MAX_CHAN_PER_UNIT                2        

#define MV_SATA_MAX_CHAN                        2

#define MV_MPP_MAX_GROUP                        9

#define MV_DRAM_MAX_CS                          4
#define MV_SPI_MAX_CS                           8
 
#ifdef MV_INCLUDE_PCI
	#define MV_PCI_MAX_IF                   1
	#define MV_PCI_START_IF                 0
	#define PCI_HOST_BUS_NUM(pciIf)         (pciIf)
	#define PCI_HOST_DEV_NUM(pciIf)         0
#else
	#define MV_PCI_MAX_IF                   0
	#define MV_PCI_START_IF                 0
#endif

#define MV_PEX_MAX_IF                           2
#define MV_PEX_MAX_UNIT                         2
#ifdef MV_INCLUDE_PEX
#define MV_INCLUDE_PEX0
#define MV_DISABLE_PEX_DEVICE_BAR

#define MV_PEX_START_IF                         MV_PCI_MAX_IF
 #define PEX_HOST_BUS_NUM(pciIf)               (pciIf)
 #define PEX_HOST_DEV_NUM(pciIf)               0
#else
 #undef MV_INCLUDE_PEX0
#endif

#define PCI_IO(pciIf)                           (PEX0_IO + 2 * (pciIf))
#define PCI_MEM(pciIf, memNum)                  (PEX0_MEM0 + 2 * (pciIf))
 
#define MV_IDMA_MAX_CHAN                        4
#define MV_DEVICE_MAX_CS                        4

#ifndef MV_USB_MAX_PORTS
#define MV_USB_MAX_PORTS 3
#endif

#define MV_CESA_VERSION                         3  
#define MV_CESA_SRAM_SIZE                       (2 * 1024)

#define MV_PP2_PON_PORT_ID			7
#define MV_ETH_MAX_PORTS                        4
#define MV_PP2_MAX_RXQ                          16 
#define MV_PP2_MAX_TXQ                          8
#define MV_PP2_RXQ_TOTAL_NUM                    32       
#define MV_PP2_MAX_TCONT                        16       
#define MV_PP2_TX_CSUM_MAX_SIZE                 1790

#define MV_USB_VERSION                          1

#define MV_SPI_VERSION                          2

#define MV_INCLUDE_SDRAM_CS0
#define  MV_INCLUDE_SDRAM_CS1
#undef  MV_INCLUDE_SDRAM_CS2
#undef  MV_INCLUDE_SDRAM_CS3

#define MV_INCLUDE_DEVICE_CS0
#define MV_INCLUDE_DEVICE_CS1
#define MV_INCLUDE_DEVICE_CS2
#define MV_INCLUDE_DEVICE_CS3

#ifndef MV_ASMLANGUAGE

#define TBL_UNUSED      0        
#define FREQ_MODES_NUM_6720	3

#define MPP_GROUP_0_TYPE { \
	0x55555555,       \
	0x00020020,       \
}

typedef enum {
	NAND_BOOT_V2,
	SPI0_BOOT,
	SPI0_BOOT_SPDIF_AUDIO,
} MV_GROUP_0_TYPE;

#define MPP_GROUP_1_TYPE { \
	0x22555555,       \
	0x22000022,       \
}

#define MV_GROUP_1_TYPE         MV_GROUP_0_TYPE

#define MPP_GROUP_2_TYPE { \
	0x22222222,       \
	0x22222222,       \
}

typedef enum {
	SLIC_SSI_DEV,
	SLIC_ISI_DEV,
	SLIC_ZSI_DEV,
	SLIC_EXTERNAL_DEV
} MV_GROUP_2_TYPE;

#define MPP_GROUP_3_TYPE { \
	0x33333333,       \
	0x33333333,       \
}

typedef enum {
	GE1_UNIT,
	SDIO_UNIT,
	SPI1_UNIT,
	SDIO_SPI1_UNIT,
} MV_GROUP_3_TYPE;

#define MPP_GROUP_4_TYPE { \
	0x00003330,       \
	0x00003330,       \
}

typedef enum {
	GE1_CPU_SMI_CTRL_TDM_LQ_UNIT,
	GE1_CPU_SMI_CTRL_REF_CLK_OUT,
} MV_GROUP_4_TYPE;

#define MPP_GROUP_5_TYPE { \
	0x22002044,       \
	0x22002044,       \
}
typedef enum {
	GE0_UNIT_PON_TX_FAULT,
	GE0_UNIT_PON_CLK_OUT,
	SWITCH_P4_PON_TX_FAULT,
	SWITCH_P4_PON_CLK_OUT,
} MV_GROUP_5_TYPE;

#define MPP_GROUP_6_TYPE { \
	0x22222222,       \
	0x22222222,       \
}

typedef enum {
	GE0_UNIT,
	SWITCH_P4,
} MV_GROUP_6_TYPE;

#define MPP_GROUP_7_TYPE { \
	0x00000022,       \
	0x00000022,       \
}

typedef enum {
	SWITCH_P4_LED_MATRIX,
	GE0_UNIT_LED_MATRIX,
	SWITCH_P4_UA1_PTP,
	GE0_UNIT_UA1_PTP,
} MV_GROUP_7_TYPE;

#define MPP_GROUP_8_TYPE { \
		0x000,\
}

typedef enum {
	LED_MATRIX_PTP,
} MV_GROUP_8_TYPE;

#define MPP_GROUP_TYPES { \
	MPP_GROUP_0_TYPE, \
	MPP_GROUP_1_TYPE, \
	MPP_GROUP_2_TYPE, \
	MPP_GROUP_3_TYPE, \
	MPP_GROUP_4_TYPE, \
	MPP_GROUP_5_TYPE, \
	MPP_GROUP_6_TYPE, \
	MPP_GROUP_7_TYPE, \
	MPP_GROUP_8_TYPE, \
}

typedef enum {
	SLIC_EXTERNAL_ID,
	SLIC_ZARLINK_ID,
	SLIC_SILABS_ID,
	SLIC_LANTIQ_ID
} MV_SLIC_UNIT_TYPE;

typedef enum {
	TDM_UNIT_2CH,
	TDM_UNIT_8CH
} MV_TDM_UNIT_TYPE;

typedef enum _mvUnitId {
	DRAM_UNIT_ID,
	PEX_UNIT_ID,
	ETH_GIG_UNIT_ID,
	USB_UNIT_ID,
	USB3_UNIT_ID,
	IDMA_UNIT_ID,
	XOR_UNIT_ID,
	SATA_UNIT_ID,
	TDM_UNIT_ID,
	UART_UNIT_ID,
	CESA_UNIT_ID,
	SPI_UNIT_ID,
	AUDIO_UNIT_ID,
	SDIO_UNIT_ID,
	TS_UNIT_ID,
	XPON_UNIT_ID,
	BM_UNIT_ID,
	PNC_UNIT_ID,
	I2C_UNIT_ID,
	SGMII_UNIT_ID,
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
	USB3,            
	CRYPT0_ENG,      
	CRYPT1_ENG,      
	PP2_CPU0,	 
	PP2_CPU1,	 
	MAX_TARGETS
} MV_TARGET;

#ifdef AURORA_IO_CACHE_COHERENCY
#define DRAM_CS0_ATTR           0x1E
#define DRAM_CS1_ATTR           0x1D
#define DRAM_CS2_ATTR           0x1B
#define DRAM_CS3_ATTR           0x17
#else
#define DRAM_CS0_ATTR           0x0E
#define DRAM_CS1_ATTR           0x0D
#define DRAM_CS2_ATTR           0x0B
#define DRAM_CS3_ATTR           0x07
#endif

#define MAIN_BOOT_ATTR         0x1D     
#define SEC_BOOT_ATTR          0x2F     

#define TARGETS_DEF_ARRAY {                                                 \
	{ DRAM_CS0_ATTR, DRAM_TARGET_ID },	  \
	{ DRAM_CS1_ATTR, DRAM_TARGET_ID },	  \
	{ DRAM_CS2_ATTR, DRAM_TARGET_ID },	  \
	{ DRAM_CS3_ATTR, DRAM_TARGET_ID },	  \
	{ 0x3E, DEV_TARGET_ID	},		  \
	{ 0x3D, DEV_TARGET_ID	},		  \
	{ 0x3B, DEV_TARGET_ID	},		  \
	{ 0x37, DEV_TARGET_ID	},		  \
	{ 0xE8, PEX_TARGET_ID	},		  \
	{ 0xE0, PEX_TARGET_ID	},		  \
	{ 0xD8, PEX_TARGET_ID	},		  \
	{ 0xD0, PEX_TARGET_ID	},		  \
	{ 0xFF, 0xFF		},		  \
	{ 0x01, DEV_TARGET_ID	},		  \
	{ 0x1E, DEV_TARGET_ID	},		  \
	{ 0x5E, DEV_TARGET_ID	},		  \
	{ 0x9E, DEV_TARGET_ID	},		  \
	{ 0xDE, DEV_TARGET_ID	},		  \
	{ 0x1F, DEV_TARGET_ID	},		  \
	{ 0x5F, DEV_TARGET_ID	},		  \
	{ 0x9F, DEV_TARGET_ID	},		  \
	{ 0xDF, DEV_TARGET_ID	},		  \
	{ MAIN_BOOT_ATTR, DEV_TARGET_ID },	  \
	{ SEC_BOOT_ATTR, DEV_TARGET_ID  },	  \
	{ 0x00, USB3_TARGET_ID },                 \
	{ 0x09, CRYPT_TARGET_ID	},		  \
	{ 0x05, CRYPT_TARGET_ID	},		  \
	{ 0x00, PP2_TARGET_ID	},		  \
	{ 0x01, PP2_TARGET_ID	},		  \
}

#define CESA_TARGET_NAME_DEF    ("CRYPT_ENG0", "CRYPT_ENG1")
#define TARGETS_NAME_ARRAY      {			\
	"SDRAM_CS0",		 		\
	"SDRAM_CS1",		 		\
	"SDRAM_CS2",		 		\
	"SDRAM_CS3",		 		\
	"DEVICE_CS0",		 	\
	"DEVICE_CS1",		 	\
	"DEVICE_CS2",		 	\
	"DEVICE_CS3",		 	\
	"PEX0_MEM",		 		\
	"PEX0_IO",		 		\
	"PEX1_MEM",		 		\
	"PEX1_IO",		 		\
	"INTER_REGS",		 	\
	"DMA_UART",		 		\
	"SPI_CS0",		 		\
	"SPI_CS1",		 		\
	"SPI_CS2",		 		\
	"SPI_CS3",		 		\
	"SPI_CS4",		 		\
	"SPI_CS5",		 		\
	"SPI_CS6",		 		\
	"SPI_CS7",		 		\
	"BOOT_ROM_CS",		 	\
	"DEV_BOOTCS",		 		\
	"USB3",                                \
	"CRYPT0_ENG",		 	\
	"CRYPT1_ENG",		 	\
	"PP2 - CPU 0",		 	\
	"PP2 - CPU 1"		 	\
}

#endif  

#ifdef __cplusplus
}
#endif   

#endif   
