

#ifndef __SYNO_H_
#define __SYNO_H_

#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif
#define SYNO_HAVE_KERNEL_VERSION(a,b,c) (LINUX_VERSION_CODE >= KERNEL_VERSION((a),(b),(c)) )
#define SYNO_HAVE_GCC_VERSION(a,b) (__GNUC__ > (a) || (__GNUC__ == (a) && __GNUC_MINOR__ >= (b)))
#define SYNO_HAVE_GLIBC_VERSION(a,b) ( __GLIBC__ > (a) || (__GLIBC__ == (a) && __GLIBC_MINOR__ >= (b)))



#define MY_ABC_HERE


#if 0 
#define MY_DEF_HERE
#endif


#if 0
#define MY_DEF_HERE
#endif


#if 0 
#define MY_DEF_HERE
#endif


#if 0 
#define MY_DEF_HERE
#endif


#if 0  
#define MY_DEF_HERE
#define SYNO_USB_FLASH_DEVICE_INDEX 255
#define SYNO_USB_FLASH_DEVICE_NAME  "synoboot"
#define SYNO_USB_FLASH_DEVICE_PATH  "/dev/synoboot"
#define SYNO_USBBOOT_ID_VENDOR  0xF400
#define SYNO_USBBOOT_ID_PRODUCT 0xF400
#endif


#if 0 
#define MY_DEF_HERE
#endif


#if 0 
#define MY_DEF_HERE
#endif


#if 0 
#define MY_DEF_HERE
#endif


#if 1 
#define MY_ABC_HERE
#endif


#if 0

#endif

#if 1 
#define MY_ABC_HERE
#endif


#if 1 
#define MY_ABC_HERE
#endif



#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#if 0  
#define MY_DEF_HERE
#endif



#define MY_ABC_HERE


#define MY_ABC_HERE


#ifdef MY_ABC_HERE
#define MY_ABC_HERE
#endif


#define MY_ABC_HERE



#define MY_ABC_HERE


#if 0 
#define MY_DEF_HERE
#endif


#if 0 
#define MY_DEF_HERE
#endif


#define MY_ABC_HERE



#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define	MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#if 0  
#define MY_DEF_HERE
#endif


#if 0
#define MY_DEF_HERE
#endif


#if 0
#define MY_DEF_HERE
#endif


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE



#if 0
#define MY_DEF_HERE
#endif




#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE
#define USBCOPY_PORT_LOCATION 99


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE

#ifdef MY_ABC_HERE
#define MY_ABC_HERE
#define SDCOPY_PORT_LOCATION 98
#endif




#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#ifdef MY_ABC_HERE
#define MY_ABC_HERE
#endif


#define MY_ABC_HERE



#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE
#define SD_IOCTL_IDLE 4746
#define SD_IOCTL_SUPPORT_SLEEP  4747
#define PORT_TYPE_SATA 1
#define PORT_TYPE_USB  2


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#ifdef MY_ABC_HERE
#define MY_ABC_HERE
#endif


#define MY_ABC_HERE


#define MY_ABC_HERE

#if	defined(MY_ABC_HERE) || defined(SYNO_BADSECTOR_TEST)

#define SYNO_MAX_INTERNAL_DISK	9
#endif


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#if 1
#define MY_ABC_HERE
#endif


#define MY_ABC_HERE


#define MY_ABC_HERE
 

#define MY_ABC_HERE



#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#if 1
#define MY_ABC_HERE
#endif


#define MY_ABC_HERE

 
#define MY_ABC_HERE

 
#define MY_ABC_HERE

 
#define MY_ABC_HERE



#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE 


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE
#ifdef MY_ABC_HERE

#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE
#endif


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE
#ifdef MY_ABC_HERE

#ifndef SYNO_MARVELL_88F6180
#define MY_ABC_HERE
#endif

#if defined (F_CLEAR_ARCHIVE) || defined (F_SETSMB_ARCHIVE) || defined (F_SETSMB_HIDDEN) || \
	defined (F_SETSMB_SYSTEM) || defined (F_CLRSMB_ARCHIVE) || defined (F_CLRSMB_HIDDEN) || \
	defined (F_CLRSMB_SYSTEM) || defined (F_CLEAR_S3_ARCHIVE)
#error "Samba archive bit redefine."
#endif

#if 1
#if defined (F_CLRSMB_READONLY) || defined (F_SETSMB_READONLY) || \
	defined (F_CLRACL_INHERIT)  || defined (F_SETACL_INHERIT)  || \
	defined (F_CLRACL_OWNER_IS_GROUP) || defined (F_SETACL_OWNER_IS_GROUP)  || \
	defined (F_SETACL_SUPPORT) || defined (F_SETACL_SUPPORT)
#error "ACL archive bit redefine."
#endif 
#endif 

#define F_CLEAR_ARCHIVE     513
#define F_SETSMB_ARCHIVE    514
#define F_SETSMB_HIDDEN     515
#define F_SETSMB_SYSTEM     516
#define F_CLRSMB_ARCHIVE    517
#define F_CLRSMB_HIDDEN     518
#define F_CLRSMB_SYSTEM     519
#define F_CLEAR_S3_ARCHIVE  520

#ifdef MY_ABC_HERE
#define F_CLRSMB_READONLY   		521
#define F_SETSMB_READONLY   		522
#define F_CLRACL_INHERIT    		523
#define F_SETACL_INHERIT    		524
#define F_CLRACL_HAS_ACL   			525
#define F_SETACL_HAS_ACL   			526
#define F_CLRACL_SUPPORT   			527
#define F_SETACL_SUPPORT   			528
#define F_CLRACL_OWNER_IS_GROUP   	529
#define F_SETACL_OWNER_IS_GROUP   	530
#endif 

#endif 

#define MY_ABC_HERE
#ifdef MY_ABC_HERE
#ifdef MY_ABC_HERE
#define MY_ABC_HERE
#endif

#define SYNO_SMB_PSTRING_LEN 1024
#endif


#define MY_ABC_HERE


#ifdef MY_ABC_HERE
#define MY_ABC_HERE
#ifdef MY_ABC_HERE


#endif
#endif


#define MY_ABC_HERE


 #define MY_ABC_HERE



#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE




#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#if 1 
#define MY_ABC_HERE
#endif


#ifndef MY_ABC_HERE
#define MY_DEF_HERE
#endif


#define MY_ABC_HERE


#if 1

#define MY_ABC_HERE


#ifdef MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE

#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE

#endif
#endif



#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif


#define MY_ABC_HERE


#define MY_ABC_HERE


#ifdef MY_ABC_HERE
#define MY_ABC_HERE
#endif


#define MY_ABC_HERE


#if 0 
#define MY_DEF_HERE
#endif


#if 1 
#define MY_ABC_HERE
#endif



#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE

#define MY_ABC_HERE

#ifdef MY_ABC_HERE
#define MAX_CHANNEL_RETRY       2
#define CHANNEL_RETRY_INTERVAL  (3*HZ)


#define MY_ABC_HERE
#endif


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE

#include <linux/syno_user.h>

#include <linux/syno_debug.h>


#define SYNO_NFSD_WRITE_SIZE_MIN 65536


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE


#define MY_ABC_HERE
#endif 
