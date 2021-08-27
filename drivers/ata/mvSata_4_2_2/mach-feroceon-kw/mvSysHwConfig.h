 
#ifndef __INCmvSysHwConfigh
#define __INCmvSysHwConfigh

#include <generated/autoconf.h>

#define CONFIG_MARVELL	1

#define _1K         0x00000400
#define _4K         0x00001000
#define _8K         0x00002000
#define _16K        0x00004000
#define _32K        0x00008000
#define _64K        0x00010000
#define _128K       0x00020000
#define _256K       0x00040000
#define _512K       0x00080000

#define _1M         0x00100000
#define _2M         0x00200000
#define _4M         0x00400000
#define _8M         0x00800000
#define _16M        0x01000000
#define _32M        0x02000000
#define _64M        0x04000000
#define _128M       0x08000000
#define _256M       0x10000000
#define _512M       0x20000000

#define _1G         0x40000000
#define _2G         0x80000000

#ifdef CONFIG_MV_INCLUDE_PEX
#define MV_INCLUDE_PEX
#endif
#ifdef CONFIG_MV_INCLUDE_TWSI
#define MV_INCLUDE_TWSI
#endif
#ifdef CONFIG_MV_INCLUDE_CESA
#define MV_INCLUDE_CESA
#endif
#ifdef CONFIG_MV_INCLUDE_GIG_ETH
#define MV_INCLUDE_GIG_ETH
#endif
#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
#define MV_INCLUDE_INTEG_SATA
#define MV_INCLUDE_SATA
#endif
#ifdef CONFIG_MV_INCLUDE_USB
#define MV_INCLUDE_USB
#define MV_USB_VOLTAGE_FIX
#endif
#ifdef CONFIG_MV_INCLUDE_NAND
#define MV_INCLUDE_NAND
#endif
#ifdef CONFIG_MV_INCLUDE_TDM
#define MV_INCLUDE_TDM
#endif
#ifdef CONFIG_MV_INCLUDE_XOR
#define MV_INCLUDE_XOR
#endif
#ifdef CONFIG_MV_INCLUDE_TWSI
#define MV_INCLUDE_TWSI
#endif
#ifdef CONFIG_MV_INCLUDE_UART
#define MV_INCLUDE_UART
#endif
#ifdef CONFIG_MV_INCLUDE_SPI
#define MV_INCLUDE_SPI
#endif
#ifdef CONFIG_MV_INCLUDE_SFLASH_MTD
#define MV_INCLUDE_SFLASH_MTD
#endif
#ifdef CONFIG_MV_INCLUDE_AUDIO
#define MV_INCLUDE_AUDIO
#endif
#ifdef CONFIG_MV_INCLUDE_TS
#define MV_INCLUDE_TS
#endif
#ifdef CONFIG_MV_INCLUDE_SDIO
#define MV_INCLUDE_SDIO
#endif

#ifdef CONFIG_MV_NAND_BOOT
#define MV_NAND_BOOT
#endif
#ifdef CONFIG_MV_NAND
#define MV_NAND
#endif

#define MV_INCLUDE_CLK_PWR_CNTRL

#define MV_DISABLE_PEX_DEVICE_BAR

#define MV_INCLUDE_EARLY_PRINTK

#ifdef MV_INCLUDE_CESA

#define MV_CESA_MAX_CHAN               4

#define MV_CESA_MAX_BUF_SIZE           1600

#endif  

#if defined(CONFIG_MV_INCLUDE_GIG_ETH)

#ifdef CONFIG_MV_NFP_STATS
#define MV_FP_STATISTICS
#else
#undef MV_FP_STATISTICS
#endif

#define MV_ETH_SKB_REUSE_DEFAULT    1

#define MV_ETH_TX_EN_DEFAULT        0

#define ETH_DESCR_IN_SDRAM
#undef  ETH_DESCR_IN_SRAM     

#if defined(ETH_DESCR_IN_SRAM)
#if defined(ETH_DESCR_UNCACHED)
 #define ETH_DESCR_CONFIG_STR    "Uncached descriptors in integrated SRAM"
#else
 #define ETH_DESCR_CONFIG_STR    "Cached descriptors in integrated SRAM"
#endif
#elif defined(ETH_DESCR_IN_SDRAM)
#if defined(ETH_DESCR_UNCACHED)
 #define ETH_DESCR_CONFIG_STR    "Uncached descriptors in DRAM"
#else
 #define ETH_DESCR_CONFIG_STR    "Cached descriptors in DRAM"
#endif
#else 
 #error "Ethernet descriptors location undefined"
#endif  
 
#ifndef MV_CACHE_COHER_SW
 
#define MV_UNCACHED             0   
 
#define MV_CACHE_COHER_HW_WT    1
 
#define MV_CACHE_COHER_HW_WB    2
 
#define MV_CACHE_COHER_SW       3

#endif

#define MV_CACHE_COHERENCY  MV_CACHE_COHER_SW

#define ETHER_DRAM_COHER    MV_CACHE_COHER_SW    

#if (ETHER_DRAM_COHER == MV_CACHE_COHER_HW_WB)
 #define ETH_SDRAM_CONFIG_STR    "DRAM HW cache coherency (write-back)"
#elif (ETHER_DRAM_COHER == MV_CACHE_COHER_HW_WT)
 #define ETH_SDRAM_CONFIG_STR    "DRAM HW cache coherency (write-through)"
#elif (ETHER_DRAM_COHER == MV_CACHE_COHER_SW)
 #define ETH_SDRAM_CONFIG_STR    "DRAM SW cache-coherency"
#elif (ETHER_DRAM_COHER == MV_UNCACHED)
#   define ETH_SDRAM_CONFIG_STR  "DRAM uncached"
#else
 #error "Ethernet-DRAM undefined"
#endif  

#define ETH_DEF_TXQ         0
#define ETH_DEF_RXQ         0 

#define MV_ETH_RX_Q_NUM     CONFIG_MV_ETH_RX_Q_NUM
#define MV_ETH_TX_Q_NUM     CONFIG_MV_ETH_TX_Q_NUM

#define ETH_TX_COAL    		    200
#define ETH_RX_COAL    		    200

#define TX_CSUM_OFFLOAD
#define RX_CSUM_OFFLOAD

#define MV_ETH_EXTRA_TX_DESCR	    20 

#if (MV_ETH_RX_Q_NUM > 1)
#define ETH_NUM_OF_RX_DESCR         64
#define ETH_RX_QUEUE_QUOTA	    32    
#else
#define ETH_NUM_OF_RX_DESCR         128
#endif

#define ETH_NUM_OF_TX_DESCR         (ETH_NUM_OF_RX_DESCR*4 + MV_ETH_EXTRA_TX_DESCR)

#endif  

#if defined(CONFIG_MV_TDM_LINEAR_MODE)
 #define MV_TDM_LINEAR_MODE
#elif defined(CONFIG_MV_TDM_ULAW_MODE)
 #define MV_TDM_ULAW_MODE
#endif

#define DRAM_BUF_REG0   0x30810    
#define DRAM_BUF_REG1   0x30820  
#define DRAM_BUF_REG2   0x30830  
#define DRAM_BUF_REG3   0x308c4            
#define DRAM_BUF_REG4   0x60a90  
#define DRAM_BUF_REG5   0x60a94  
#define DRAM_BUF_REG6   0x60a98  
#define DRAM_BUF_REG7   0x60a9c  
#define DRAM_BUF_REG8   0x60b90  
#define DRAM_BUF_REG9   0x60b94  
#define DRAM_BUF_REG10  0x60b98  
#define DRAM_BUF_REG11  0x60b9c  
#define DRAM_BUF_REG12  0x60a00  
#define DRAM_BUF_REG13  0x60a04  
#define DRAM_BUF_REG14  0x60b00  

#define DRAM_BUF_REG_DV 0

#ifdef CONFIG_SYNO_MV88F6281
#define SDRAM_CS0_BASE  0x00000000
#define SDRAM_CS0_SIZE  _128M

#define SDRAM_CS1_BASE  0x08000000
#define SDRAM_CS1_SIZE  _128M
#else
#define SDRAM_CS0_BASE  0x00000000
#define SDRAM_CS0_SIZE  _256M

#define SDRAM_CS1_BASE  0x10000000
#define SDRAM_CS1_SIZE  _256M
#endif  

#define SDRAM_CS2_BASE  0x20000000
#define SDRAM_CS2_SIZE  _256M

#define SDRAM_CS3_BASE  0x30000000
#define SDRAM_CS3_SIZE  _256M

#define PEX0_MEM_BASE 0xe8000000
#define PEX0_MEM_SIZE _128M

#define PEX0_IO_BASE 0xf2000000
#define PEX0_IO_SIZE _1M

#define NFLASH_CS_BASE 0xfa000000
#define NFLASH_CS_SIZE _2M

#ifdef CONFIG_SYNO_MV88F6281
#define SPI_CS_BASE 0xf8000000
#define SPI_CS_SIZE _4M
#else
#define SPI_CS_BASE 0xf4000000
#define SPI_CS_SIZE _16M
#endif  

#define CRYPT_ENG_BASE	0xf0000000
#define CRYPT_ENG_SIZE	_2M

#define BOOTDEV_CS_BASE	0xff800000
#define BOOTDEV_CS_SIZE _8M

#define DEVICE_CS2_BASE 0xff900000
#define DEVICE_CS2_SIZE _1M

#define PEX_CONFIG_RW_WA_TARGET PEX0_MEM
 
#define PEX_CONFIG_RW_WA_USE_ORIGINAL_WIN_VALUES 1
 
#define PEX_CONFIG_RW_WA_BASE 0xF3000000
#define PEX_CONFIG_RW_WA_SIZE _16M

#define INTER_REGS_BASE	0xF1000000

#define MV_DRAM_AUTO_SIZE

#undef TCLK_AUTO_DETECT    	 
#define SYSCLK_AUTO_DETECT	 
#define PCLCK_AUTO_DETECT  	 
#define L2CLK_AUTO_DETECT 	 

#define PCI0_IF_PTP		0		 

#endif  
