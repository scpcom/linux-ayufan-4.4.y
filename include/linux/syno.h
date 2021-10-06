#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __SYNO_H_
#define __SYNO_H_

#ifdef	MY_ABC_HERE
#define IS_SYNO_USBBOOT_ID_VENDOR(VENDOR) (0xF400 == (VENDOR))
#define IS_SYNO_USBBOOT_ID_PRODUCT(PRODUCT) (0xF400 == (PRODUCT))
#endif  

#if defined(CONFIG_ARCH_FEROCEON) && defined(CONFIG_SATA_MV)
#define SYNO_6281_SOC_USE_OPENSOURCE_SATA
#endif

#if defined(CONFIG_ARCH_FEROCEON) || defined(CONFIG_ARCH_KIRKWOOD) || defined(CONFIG_ARCH_ARMADA370)
#define SYNO_ESATA_7042
#endif

#ifdef CONFIG_SYNO_MV88F6281
#define SYNO_FLASH_MEMORY_SIZE
#endif

#ifdef MY_ABC_HERE
#define SYNO_YOTAWIMAX_DESC          "SYNO CDC Ethernet Device for YotaKey"
#define SYNO_YOTAWIMAX_ETHERNET_NAME "wm"
#define SYNO_YOTAWIMAX_NET_NOLINK_EVENT (0xffffffff)
#endif

#ifdef CONFIG_SYNO_MPC85XX_COMMON
#define SYNO_NET_PHY_NOLINK_SPEED_INIT
#endif

#ifdef CONFIG_MACH_SYNOLOGY_6281
#define SYNO_6281_MTU_WA
#endif

#if defined(CONFIG_PPC_85xx)
#define SYNO_IPV6_110p_IPV6_READY
#endif

#if 0
 
#define SYNO_USB_STOR_COMP_ENHANCE
#endif
 
#if 0
 
#define SYNO_USB3_TIMEOUT
#endif
#if 0
#define SYNO_USB3_RESET_RETRY
#define SYNO_USB3_STALL_WAIT
#endif
 
#if 0
 
#define SYNO_USB3_DEBUG

#define SYNO_USB3_ERR_MONITOR
#endif

#if 0
 
#define SYNO_USB3_SPECIAL_RESET
#endif

#if 0
 
#define SYNO_USB3_RESET_FOR_ADDR_ERR
#endif
 
#define SYNO_FIX_MD_RESIZE_BUSY_LOOP 5

#ifdef CONFIG_SYNO_MV88F6281
#define SYNO_LIBATA_JMB_BEHAVIOR
#endif

#ifdef MY_DEF_HERE
#define SYNO_MV_9170_GPIO_CTRL
#endif  

#ifdef MY_ABC_HERE
#define SYNO_SMB_PSTRING_LEN 1024
#endif

#if defined(CONFIG_MV_XOR_MEMCOPY) && (defined(MY_DEF_HERE) || defined(MY_ABC_HERE))
#define SYNO_MV_PERF
#endif  

#ifdef CONFIG_SYNO_MV88F6281_USBSTATION
#define SYNO_SLOW_DOWN_UEVENT
#endif

#if defined(CONFIG_SYNO_MV88F6281) && !defined(CONFIG_SYNO_MV88F6281_USBSTATION)
#define SYNO_CPUFREQ_ADJUST
#endif

#ifdef MY_ABC_HERE
#define MAX_CHANNEL_RETRY       2
#define CHANNEL_RETRY_INTERVAL  (3*HZ)

#endif

#include <uapi/linux/syno.h>

#ifdef MY_ABC_HERE
 
#define SYNO_ISCSI_DEVICE_INDEX    (26 + 25 * 26)     

#ifdef MY_DEF_HERE
 
#define SYNO_INTERNAL_MICROSD_NAME "4-4"
#endif  
#endif  

#if defined(CONFIG_SYNO_MV88F6281) || defined(SYNO_MARVELL_88F6180) || defined(SYNO_MARVELL_88F6281)
#define SYNO_ARM_GENERIC_ATOMIC64
#endif

#endif  
