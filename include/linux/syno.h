#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __SYNO_H_
#define __SYNO_H_

#ifndef __KERNEL__
#include <linux/syno_autoconf.h>
#endif

#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif
#define SYNO_HAVE_KERNEL_VERSION(a,b,c) (LINUX_VERSION_CODE >= KERNEL_VERSION((a),(b),(c)) )
#define SYNO_HAVE_GCC_VERSION(a,b) (__GNUC__ > (a) || (__GNUC__ == (a) && __GNUC_MINOR__ >= (b)))
#define SYNO_HAVE_GLIBC_VERSION(a,b) ( __GLIBC__ > (a) || (__GLIBC__ == (a) && __GLIBC_MINOR__ >= (b)))

#if defined(MY_ABC_HERE)
#define CONSISTENT_DMA_SIZE 0x00800000
#endif

#ifdef CONFIG_SYNO_PLX_PORTING 
#define SYNO_FAST_RW_FIX
#endif

#if defined(MY_DEF_HERE)
#define SYNO_X86_AUTO_POWER_ON
#endif

#if !defined(CONFIG_SYNO_MPC85XX_COMMON)
#define SYNO_USB_FLASH_DEVICE_INDEX 255
#define SYNO_USB_FLASH_DEVICE_NAME  "synoboot"
#define SYNO_USB_FLASH_DEVICE_PATH  "/dev/synoboot"
 
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#define IS_SYNO_USBBOOT_ID_VENDOR(VENDOR) (0xF400 == (VENDOR) || 0xF401 == (VENDOR))
#define IS_SYNO_USBBOOT_ID_PRODUCT(PRODUCT) (0xF400 == (PRODUCT) || 0xF401 == (PRODUCT))
#else
#define IS_SYNO_USBBOOT_ID_VENDOR(VENDOR) (0xF400 == (VENDOR))
#define IS_SYNO_USBBOOT_ID_PRODUCT(PRODUCT) (0xF400 == (PRODUCT))
#endif
 
#endif  

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#define SYNO_PCH_GPIO_CTRL
#endif

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#define SYNO_PINEVIEW_RTC
#endif

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#define SYNO_PINEVIEW_CORETEMP
#define SYNO_BROMOLOW_CORETEMP
#define SYNO_CEDARVIEW_CORETEMP
#endif

#if defined(CONFIG_SYNO_MPC854X) || defined(MY_DEF_HERE) || defined(MY_DEF_HERE) || defined(CONFIG_ARCH_FEROCEON)

#ifdef MY_ABC_HERE
#define SYNO_MAX_SWITCHABLE_NET_DEVICE 8
#define SYNO_NET_DEVICE_ENCODING_LENGTH 6
#endif   
#endif

#ifdef CONFIG_SYNO_MPC85XX_COMMON
#define SYNO_NET_PHY_NOLINK_SPEED_INIT
#endif

#ifdef CONFIG_MACH_SYNOLOGY_6281
#define SYNO_6281_MTU_WA
#endif

#define USBCOPY_PORT_LOCATION 99

#ifdef MY_ABC_HERE
#define SDCOPY_PORT_LOCATION 98
#endif

#ifdef MY_ABC_HERE
#define CHECKINTERVAL (7UL*HZ)
#endif

#if	defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
 
#if defined(MY_DEF_HERE)
#define SYNO_MAX_INTERNAL_DISK 19
#else
#define SYNO_MAX_INTERNAL_DISK	15
#endif

#endif

#if defined(CONFIG_SYNO_MPC8533) || defined(MY_DEF_HERE)
#ifdef MY_ABC_HERE
#ifdef MY_DEF_HERE
#define SYNO_CREATE_TIME_SWAP_VERSION 3719
#endif
#endif
#endif

#ifdef MY_ABC_HERE

#if defined (F_CLEAR_ARCHIVE) || defined (F_SETSMB_ARCHIVE) || defined (F_SETSMB_HIDDEN) || \
	defined (F_SETSMB_SYSTEM) || defined (F_CLRSMB_ARCHIVE) || defined (F_CLRSMB_HIDDEN) || \
	defined (F_CLRSMB_SYSTEM) || defined (F_CLEAR_S3_ARCHIVE)
#error "Samba archive bit redefine."
#endif

#if defined(MY_ABC_HERE) || defined(CONFIG_FS_SYNO_ACL)
#if defined (F_CLRSMB_READONLY) || defined (F_SETSMB_READONLY) || \
	defined (F_CLRACL_INHERIT)  || defined (F_SETACL_INHERIT)  || \
	defined (F_CLRACL_OWNER_IS_GROUP) || defined (F_SETACL_OWNER_IS_GROUP)  || \
	defined (F_SETACL_SUPPORT) || defined (F_SETACL_SUPPORT)
#error "ACL archive bit redefine."
#endif  
#endif  

#define SYNO_FCNTL_BASE             513
#define F_CLEAR_ARCHIVE             (SYNO_FCNTL_BASE + 0)
#define F_SETSMB_ARCHIVE            (SYNO_FCNTL_BASE + 1)
#define F_SETSMB_HIDDEN             (SYNO_FCNTL_BASE + 2)
#define F_SETSMB_SYSTEM             (SYNO_FCNTL_BASE + 3)
#define F_CLRSMB_ARCHIVE            (SYNO_FCNTL_BASE + 4)
#define F_CLRSMB_HIDDEN             (SYNO_FCNTL_BASE + 5)
#define F_CLRSMB_SYSTEM             (SYNO_FCNTL_BASE + 6)
#define F_CLEAR_S3_ARCHIVE          (SYNO_FCNTL_BASE + 7)

#ifdef MY_ABC_HERE
#define F_CLRSMB_READONLY           (SYNO_FCNTL_BASE + 8)
#define F_SETSMB_READONLY           (SYNO_FCNTL_BASE + 9)
#define F_CLRACL_INHERIT            (SYNO_FCNTL_BASE + 10)
#define F_SETACL_INHERIT            (SYNO_FCNTL_BASE + 11)
#define F_CLRACL_HAS_ACL            (SYNO_FCNTL_BASE + 12)
#define F_SETACL_HAS_ACL            (SYNO_FCNTL_BASE + 13)
#define F_CLRACL_SUPPORT            (SYNO_FCNTL_BASE + 14)
#define F_SETACL_SUPPORT            (SYNO_FCNTL_BASE + 15)
#define F_CLRACL_OWNER_IS_GROUP     (SYNO_FCNTL_BASE + 16)
#define F_SETACL_OWNER_IS_GROUP     (SYNO_FCNTL_BASE + 17)
#define SYNO_FCNTL_LAST             F_SETACL_OWNER_IS_GROUP
#else
#define SYNO_FCNTL_LAST             F_CLEAR_S3_ARCHIVE
#endif  

#endif  
 
#ifdef MY_ABC_HERE
 
#define SYNO_SMB_PSTRING_LEN 1024
#endif

#ifdef CONFIG_SYNO_MV88F6281_USBSTATION
#define SYNO_SLOW_DOWN_UEVENT
#endif

#ifdef MY_ABC_HERE
#define MAX_CHANNEL_RETRY       2
#define CHANNEL_RETRY_INTERVAL  (3*HZ)

#endif

#include <linux/syno_user.h>

#include <linux/syno_debug.h>

#define SYNO_NFSD_WRITE_SIZE_MIN 131072

#define SYNO_EXT4_SYNC_DALLOC_RETRY  100

#ifdef MY_ABC_HERE
#define SYNO_NFSD_UDP_MAX_PACKET_SIZE 32768
#define SYNO_NFSD_UDP_MIN_PACKET_SIZE 4096
#define SYNO_NFSD_UDP_DEF_PACKET_SIZE 8192
#endif

#ifdef MY_ABC_HERE
#define SYNO_SATA_DEVICE_PREFIX	   "sd"
#define SYNO_ISCSI_DEVICE_PREFIX   "isd"
#define SYNO_ISCSI_DEVICE_INDEX    (26 + 25 * 26)     
#endif

#ifdef MY_DEF_HERE
 
#define SYNO_SAS_MPT2_HOTPLUG_PHY

#define SYNO_SAS_RECOVER_REMOVED_ENCS

#define SYNO_SAS_SHOW_DISK_PHY_INFO

#if defined(MY_ABC_HERE)
#define SYNO_SAS_DISK_NAME
#endif

#if defined(SYNO_SAS_DISK_NAME)
#define SYNO_SAS_USB_DEVICE_PREFIX		"usb"
#define SYNO_SAS_DEVICE_PREFIX			"sas"
#ifdef MY_ABC_HERE
#define SYNO_SAS_ISCSI_DEVICE_PREFIX	"iscsi"
#endif
#endif  

#define SYNO_SAS_DISK_LED_CONTROL

#endif  

#ifdef MY_ABC_HERE
 
#define SYNO_WIFI_INTERFACE		"wlan"
#endif

#endif  
