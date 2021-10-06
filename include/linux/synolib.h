#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
// Copyright (c) 2000-2015 Synology Inc. All rights reserved.
#ifndef __SYNOLIB_H_
#define __SYNOLIB_H_

#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/fs.h>

#ifdef  MY_ABC_HERE
extern int gSynoDebugFlag;
extern int gSynoAtaDebug;
extern int gSynoHibernationLogLevel;
extern struct mm_struct *syno_get_task_mm(struct task_struct *task);
void syno_do_hibernation_fd_log(const int fd);
void syno_do_hibernation_filename_log(const char __user *filename);
void syno_do_hibernation_inode_log(struct inode *inode);
void syno_do_hibernation_bio_log(const char *DeviceName);
void syno_do_hibernation_scsi_log(const char *DeviceName);
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
int SynoSCSIGetDeviceIndex(struct block_device *bdev);
#endif
#ifdef MY_DEF_HERE
int SynoNVMeGetDeviceIndex(struct block_device *bdev);
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
/**
 * How to use :
 * 1. module itself register the proprietary instance into the kernel
 *    by a predined MAGIC-key.
 * 2. Others can query the module registration by the same MAGIC-key
 *    and get the instance handle.
 * ********************************************************************
 * Beware of casting/handing "instance", you must know
 * what you are doing before accessing the instance.
 * ********************************************************************
 */
/* For plugin-instance registration */
int syno_plugin_register(int plugin_magic, void *instance);
int syno_plugin_unregister(int plugin_magic);
/* For getting the plugin-instance */
int syno_plugin_handle_get(int plugin_magic, void **hnd);
void * syno_plugin_handle_instance(void *hnd);
void syno_plugin_handle_put(void *hnd);

/* Magic definition */
#define EPIO_PLUGIN_MAGIC_NUMBER    0x20120815
#define RODSP_PLUGIN_MAGIC_NUMBER    0x20141111
#endif

#ifdef MY_ABC_HERE
/* Maximum number of MAC addresses */
#define SYNO_MAC_MAX_NUMBER 8
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
#define SATA_REMAP_MAX  32
#define SATA_REMAP_NOT_INIT 0xff
extern int g_syno_sata_remap[SATA_REMAP_MAX];
extern int g_use_sata_remap;
int syno_get_remap_idx(int origin_idx);
extern int g_syno_mv14xx_remap[SATA_REMAP_MAX];
extern int g_use_mv14xx_remap;
int syno_get_mv_14xx_remap_idx(int origin_idx);
#endif /* MY_DEF_HERE */

#ifdef MY_DEF_HERE
#define DT_INTERNAL_SLOT "internal_slot"
#define DT_ESATA_SLOT "esata_port"
#define DT_PCIE_SLOT "pcie_slot"
#define DT_USB_SLOT "usb_slot"
#define DT_HUB_SLOT "usb_hub"
#define DT_POWER_PIN_GPIO "power_pin_gpio"
#define DT_DETECT_PIN_GPIO "detect_pin_gpio"
#define DT_HDD_ORANGE_LED "led_orange"
#define DT_HDD_GREEN_LED "led_green"
#define DT_HDD_ACT_LED "led_activity"
#define DT_SYNO_GPIO "syno_gpio"
#define DT_PCIE_ROOT "pcie_root"
#define DT_ATA_PORT "ata_port"
#define DT_AHCI "ahci"
#define DT_AHCI_RTK "ahci_rtk"
#define DT_AHCI_MVEBU "ahci_mvebu"
#define DT_MV14XX "mv14xx"
#define DT_PHY "phy"
#define DT_USB2 "usb2"
#define DT_USB3 "usb3"
#define DT_USB_PORT "usb_port"
#define DT_USB_HUB "usb_hub"
#define DT_VBUS "vbus"
#define DT_SHARED "shared"
#define DT_SYNO_SPINUP_GROUP "syno_spinup_group"
#define DT_SYNO_SPINUP_GROUP_DELAY "syno_spinup_group_delay"
#define DT_HDD_POWERUP_SEQ "syno_hdd_powerup_seq"

#define SYNO_DTS_PROPERTY_CONTENT_LENGTH 50

/* This enum must sync with synosdk/fs.h for user space having same DISK_PORT_TYPE mapping */
typedef enum _tag_DISK_PORT_TYPE{
	UNKNOWN_DEVICE = 0,
	INTERNAL_DEVICE,
	EXTERNAL_SATA_DEVICE,
	EUNIT_DEVICE,
	EXTERNAL_USB_DEVICE,
	SYNOBOOT_DEVICE,
	ISCSI_DEVICE,
	CACHE_DEVICE,
	USB_HUB_DEVICE,
	SDCARD_DEVICE,
	INVALID_DEVICE,
	DISK_PORT_TYPE_END,
} DISK_PORT_TYPE;

#endif /* MY_DEF_HERE */

#ifdef MY_DEF_HERE
#define PCI_ADDR_LEN_MAX 9
#define PCI_ADDR_NUM_MAX CONFIG_SYNO_MAX_PCI_SLOT
extern char gszPciAddrList[PCI_ADDR_NUM_MAX][PCI_ADDR_LEN_MAX];
extern int gPciAddrNum;
extern int syno_check_on_option_pci_slot(struct pci_dev *pdev);
#endif /* MY_ABC_HERE */

#ifdef MY_DEF_HERE
/* Max 768 */
#define M2SATA_START_IDX 300
extern int gPciDeferStart;
extern int g_nvc_map_index;
extern int g_syno_nvc_index_map[SATA_REMAP_MAX];
void syno_insert_sata_index_remap(unsigned int idx, unsigned int num, unsigned int id_start);
#endif /* MY_DEF_HERE */
#ifdef MY_DEF_HERE
#define M2_HOST_LEN_MAX 128
#define M2_PORT_NO_MAX 16
extern char gSynoM2HostName[M2_HOST_LEN_MAX];
extern unsigned long gSynoM2PortNo;
extern unsigned long gSynoM2PortIndex[M2_PORT_NO_MAX];
#endif /* MY_DEF_HERE */
#ifdef MY_DEF_HERE
#define SYNO_SPINUP_GROUP_MAX 16
#define SYNO_SPINUP_GROUP_PIN_MAX_NUM 8
extern int g_syno_rp_detect_no;
extern int g_syno_rp_detect_list[SYNO_SPINUP_GROUP_PIN_MAX_NUM];
extern int g_syno_hdd_detect_no;
extern int g_syno_hdd_detect_list[SYNO_SPINUP_GROUP_PIN_MAX_NUM];
extern int g_syno_hdd_enable_no;
extern int g_syno_hdd_enable_list[SYNO_SPINUP_GROUP_PIN_MAX_NUM];
#endif /* MY_DEF_HERE */
#ifdef MY_DEF_HERE
#define SYNO_DISK_LATENCY_RANK_NUM 10
#endif /* MY_DEF_HERE */
#endif //__SYNOLIB_H_
