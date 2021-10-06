#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/* Copyright (c) 2015 Synology Inc. All rights reserved. */

#ifndef __SYNO_H_
#define __SYNO_H_

/******************************* Platform Arch ************************************/
/**
 * Fix: no bug entry
 * Dsc: support hibernate to disk
 */

/**
 * Fix: no bug entry
 * Dsc: support x86 system temperature r/w
 */

/**
 * Fix: no bug entry
 * Dsc: support pineview superio pins r/w.
 *      writable pins are defined in table writable_pin_setting[].
 */

/**
 * We use 0xF400 as synoboot device
 */
#ifdef	MY_ABC_HERE
#define IS_SYNO_USBBOOT_ID_VENDOR(VENDOR) (0xF400 == (VENDOR))
#define IS_SYNO_USBBOOT_ID_PRODUCT(PRODUCT) (0xF400 == (PRODUCT))
#endif /* MY_ABC_HERE */

/**
 * Fix: no bug entry
 * Dsc: support reading CPU core temperature
 */

/**
 * Dsc: Let 6281 BSP can use sata_mv to drive the soc sata controller
 */

/**
 * Dsc: Let 6281 soc register before 7042.
 */

/**
 * Dsc: Supporting EUP standby mode control.
 * Switch EUP standby mode via superI/O.
 * Using synobios to determine HW could or not fully support EUP.
 * It means wake on LAN and auto poweron can both works fine in EUP mode.
 */

/******************************* Kernel Core **************************************/
/**
 * Fix: DSM 3.1 bug #19092
 * Dsc: Don't print message while cpu frequency changing
 * IN : arch/arm/mach-feroceon-kw/cpufreq.c
 */

/******************************* Bootloader ***************************************/
/**
 * Dsc: Pass hardware version from uboot or other boot loader.
 *      So user mode can get the hardware version by using the
 *      following command:
 *    $ cat /proc/sys/kernel/syno_hw_version
 */

/**
 * Dsc: Pass hardware revision from uboot or other boot loader.
 *      So user mode can get the hardware version by using the
 *      following command:
 *    $ cat /proc/sys/kernel/syno_hw_revision
 */

/*
 * Dsc: Pass size of flash memory from uboot or other boot loader.
 *		The default value is 4MB in 88f628x platform.
 *      So user mode can get the this information by using the
 *      following command:
 *    $ cat /proc/sys/kernel/syno_flash_mem_size
 *
 */

/**
 * Dsc: Pass hardware version from uboot or other boot loader for factory purpose.
 *      There are two parameters need to pass - syno_usb_fast_reset and syno_disable_usb3.
 *      The Armada370 builds in the USB drivers , so we can only pass the parameters before launching the kernel.
 *      So whether the USB drivers are build-in or not , we use the same way - pass parameters by uboot or grub.
 *    $ cat /proc/sys/kernel/syno_usb_fast_reset
 *    $ cat /proc/sys/kernel/syno_disable_usb3
 *
 * MY_ABC_HERE :
 *    in factory test, some flashs do not response sometimes,
 *    and the original timeout is 30/60s(SD_TIMEOUT), it is too long for factory test.
 *    So let the timeout as 3s for factory test.
 *    IN. block/blk-timeout.c, drivers/usb/host/xhci.c
 *
 * MY_ABC_HERE :
 *    for factory test, disable usb3, only usb2
 */

/******************************* MTD Driver ***************************************/
/**
 * Dsc: add mtd->lock and mtd->unlock function
 */

/**
 * Dsc: Malloc before mtd write.
 * Dep: MY_DEF_HERE
 */

/**
 * Dsc: For Synology flash. While parsing redboot fis directory, can not sort by
 *      mtd partition starting address. Should keeps the order in fis directory,
 *      ie, ppcboot/redboot maps to mtd0, zImage maps to mtd1.
 */

/******************************* Serial Driver ************************************/
/**
 * Dsc: Support for directly accessing the serial ttys device.
 *      We use syno_ttys_write() to command uP (/dev/ttyS1) in scemd/synobios
 *      to prevent breaking disk hibernation because it will bypass the vfs layer.
 */

/**
 * Dsc:
 *  Export symbol of syno_test_list for synobios.
 * IN: kernel/sysctl.c
 */

/******************************* Network Driver ***********************************/
/**
 * Dsc: Add WOL support to Marvell BSP net driver.
 */

/**
 * Dsc: Add RGMII capability/setting to Freescale TSEC(Gianfar) driver.
 */

/**
 * Dsc: change MgmtClk in MIIMCFG from 7 to 3
 * 		14.5.3.6.6 MII Management Configuration Register (MIIMCFG)
 */

/**
 * Dsc: Fix marvell 8001 network chip bug. The rx and tx checksum field is swapped.
 *      Affected Models: <ppc824x>, <ppc853x:DS109+, DS209+, DS409+>
 */

/**
 * Dsc: Fix Marvell 88E8001 driver link state bug. Set carrier state after
 *      netdev is registered so userspace can correctly get link status by
 *      checking "RUNNING" state of interface.
 */

/**
 * Dsc: Fix 853x, 854x gianfar driver link state bug. Set carrier state after
 *      netdev is registered so userspace can correctly get link status by
 *      checking "RUNNING" state of interface.
 */

/**
 * Dsc: Fix marvell 8071 network chip bug. The rx and tx checksum field is swapped.
 *      Affected Models: <mv6281:DS110j, DS210j>
 */

/**
 * Fix: #1037
 * Dsc: change sk98lin's blinking behavior.
 */

/**
 * Fix: #9579
 * Dsc: change mv8071's blinking behavior
 */

/**
 * Fix: #10984
 * Dsc: On pineview platform, some rx_missed_errors are reported
 *      during stressing. That is because the NAPI buffer is not
 *      sufficient between polling. Thus we enlarge the napi weight
 *      from 64 to 128.
 */

/**
 * Fix: #14746
 * Dsc: the ethtool always report e1000e's wol being not supported
 *      this fixes the problem.
 *	x86 platforms do not need to enable this definition,
 *	853x platforms needs this definition to make wol function
 *	     work.
 */

/**
 * DSM 3.0 #19064
 * Dsc: Support yota usb wimax dongle name.
 */
#ifdef MY_ABC_HERE
#define SYNO_YOTAWIMAX_DESC          "SYNO CDC Ethernet Device for YotaKey"
#define SYNO_YOTAWIMAX_ETHERNET_NAME "wm"
#define SYNO_YOTAWIMAX_NET_NOLINK_EVENT (0xffffffff)
#endif

/**
 * Fix: DSM #30328
 * Dsc: Fix MPC854x ERRATA eTSEC #79:
 *      Generation of Ethernet pause frames may cause Tx lockup and false BD close
 * Dsc: Fix MPC854x ERRATA eTSEC #49:
 *      Tx IP and TCP/UDP Checksum Generation not supported for some Tx FCB offsets
*/

/**
 * Fix: DS2.0 #4712, DS2.0 #4713, LKP2.6.32 #162
 * Dsc: if network cable is not plugged in, system
 *      will try slower speed to detect link. However, we
 *      should set link status to "off" and speed to "0" when
 *      autoneg state is timeout to let upper application to
 *      know "link is not available"
 */
#ifdef CONFIG_SYNO_MPC85XX_COMMON
#define SYNO_NET_PHY_NOLINK_SPEED_INIT
#endif

/**
 * Dsc: 6281 has MTU issue when MTU larger than 1600. This fix that.
 */
#ifdef CONFIG_MACH_SYNOLOGY_6281
#define SYNO_6281_MTU_WA
#endif

/**************************** BUG please fix it in the future ****************************/
/**
 * TODO
 * Dsc: for RFC 4862 after 2 hours timeout kill address
 *		because this scheme doesn't work over 110p, so modify jiffies
 */

/******************************* USB Driver ***************************************/
/**
 * Dsc: hid ups will use user mode driver, so kernel hid module will not bind the hid ups
 */

/**
 * Dsc: When keyboard or mouse plugged in, we will make it has static minor
 *      number from hiddev5.
 *      This is for our RF remote controller. The controller is keyboard+mouse.
 */

/**
 * Dsc: When there is no serial in USB device, we will generate
 *      a serial from product. Futhermore, when product is empty also,
 *      we will generate a dummy name for it. If there are the same
 *      serial in other USB device, we will add number in the suffix.
 */

/**
 * Dsc: For USBIP project. Modify code for multiconnection.
 */

#if 0
/**
 * Dsc: This is a very dirty compatibility fix for USB compatibility with
 *      JMicron USB chip (and others). We slow down its speed for more compatibility.
 */
#endif
/**
 * Dsc:
 */

/**
 * Fix: DSM #27097
 * Dsc: during poweroff, sometimes after disable device,
 * khubd want to handle connection, then null access
 */

/**
 * Fix : DSM #39549
 * Dsc : during backuping , some devices will be ejected
 */

#if 0
/**
 * Dsc: fix xhci compatibility problem
 */
#endif
/**
 * Dsc: move some warning messages to xhci_dbg
 */

/**
 * Fix: DSM #27872, #44551 can not detect WD passport/element on NEC
 * Dsc: check mapping port status when needed
 */

#if 0
/**
 * Dsc: add more debug message for usb3 connection
 */

/**
 * Fix: DSM #27872, can not detect WD passport
 * Dsc: create another xhci monitor for error handle (polling xhci port status, not interrupt like khubd),
 *        and  check mapping port status when needed
 */
#endif

#if 0
/**
 * Fix: DSM #26764, usb3 flash (Innostor) speed is not super after reboot
 * Dsc: try to do special reboot at boot
 */
#endif

/**
 * Fix: DSM #52549 , #50493 , re-enable the special reset mechanism.
 * Dsc: Since define MY_DEF_HERE is a wide-range fix , we only use part of it.
 *      So we name it as a lightweight version.
 */

#if 0
/**
 * Fix: DSM #28178, Transcend 640G reboot issue
 * Dsc: do reset if set address error
 */
#endif
/**
 * Dsc: usb3 pci id define
 */

/**
 * Dsc: some ups needs more time during address set
 */

/**
 * Fix: DSM #38346, CyberSlim dock USB_STOR_TRANSPORT_ERROR issue
 * Dsc: Limit the MAX_BURST to static 1 , cause this device's responce is slow
 */

/**
 * This flag is that we fix etron's bug or something not suitable for us.
 * All these fixes will only be in etron's official files with et prefix ,
 * ex.ethub.c , etxhci.c ... etc.
 */

/******************************* Compact DTV  ****************************************/
/**
 * Fix: Video Station #1218
 * Dsc: This flag is for the patch which handle ioctl:FE_SET_PROPERTY on 64 bit platform.
 */

/******************************* MD Driver ****************************************/
/**
 * Fix: DSM #34979
 * Dsc: Add BLKFLSBUF, BLKROSET, RAID_VERSION ioctl cmd to white list
 */

/**
 * Dsc: Allow targets to change queue limits without underlying device
 *
 *      blk_dev_issue_discard will check queue limits to decides issue
 *      discard or not. Without this patch, blk_dev_issue_discard have
 *      no chance to send discard command to eplun.
 */

/**
 * Fix: DSM #44818
 * Dsc: expand volume fail due to get incorrect block size when e2fsprogs
 * call ioctl BLKGETSIZE64.
 */
#define SYNO_FIX_MD_RESIZE_BUSY_LOOP 5

/******************************* SATA Driver **************************************/
/**
 * Fix: DSM #45516, #99931
 * Dsc: update diskstats for md device
 */

/**
 * Fix: DSM #45916
 * Dsc: /sys/class/ata_link/linkX/sata_spd will not shows
 * "6.0 Gbps" even the link speed of disk is SATA 6G.
 */

/**
 * Fix: No Bug Entry
 * Dsc: added features to control the leds via ICH9
 */

/**
 * Fix: #27837, #27954
 * Dsc: added features to switch the achi led on and off
 * Yet: kernel/sysctl.c include/linux/libata.h drivers/ata/ahci.c drivers/ata/libata-core.c
 */

/**
 * Fix: #21684
 * Dsc: Force set some disk to 1.5Gbps in driver mode to enhance compatibility.
 * Disks:Hitachi HDS723020BLA642
 */

/**
 * Fix: #19706
 * Dsc: enlarge noise threshold to prevent recieve too many noise while normal stress.
 */

/**
 * Fix: #18974, #18975, #18976
 * Dsc: Fix 3726 cannot detect hotplug event in all sata chips
 */

/**
 * Fix: #18827, #19394
 * Dsc: Make 7042 can control phy gpio
 * IN : sata_mv.c
 */

/**
 * Dsc: Provide funcSYNOGetHwCapability to synobios
 */

/**
 * Fix: DS 2.0 # 7761
 * Dsc: Send ebox refresh event throught synobios to
 *      scemd for the ebox hotplug event.
 */

#ifdef MY_ABC_HERE
/* one item is Nxxxx */
#define SYNO_PWRPIN_ITEM_LEN 1 + CONFIG_SYNO_PWRPIN_ENCODE_LEN*2
#endif /* MY_ABC_HERE */

/**
 * Let JMB could has the same behavior as other sat controller in 6281 platform
 * My test: JMB + pineview with the same kernel would not has the same problem.
 * (Can not probe DX5 when booting if the power of DX5is on)
 */

/**
 *  Dsc: Add a framework for SATA disk led control sys interface
 *		 If the function pointer has been hooked, then the sysfs of scsi disk will be able to set disk led
 *		 It will be helpful for product development when bring up
 */

/**
 *	Fix: DSM #46891
 *	Dsc: mv9235 has different gpio control machenism, so we add new function here
 */

/**
 *	Fix: DSM #89201
 *	Dsc: mv9170 has different gpio control machenism, so we add new function here
 */

/******************************* File System **************************************/
/**
 * Fix: DSM #47400
 * Dsc: Back port btrfs from 3.11 to 3.2
 */

#ifdef MY_ABC_HERE
#define SYNO_SMB_PSTRING_LEN 1024
#endif

/**
 * Fix: DS2.0 bug #14502
 * Dsc: Prevent possible dead-lock.
 */

#ifdef MY_DEF_HERE

/**
 * Fix bug #25749
 * The "Native OS" in SMB packets sent by cifs.mount will be replaced with "Linux version 2.6.32".
 * However, we have to identify those packets to export @eaDir for such clients.
 * Therefore we change the Native OS to "Synology Linux version 2.6.32" for identification.
 */

#endif

/**
 * Fix bugs [DSM] #29392
 * Force tree reconnect iocharset=utf8 to fix failed reconnecting to remote cifs share with non-ascii
 * share folder names
 */

/**
 * Fix bugs [DSM] #29931
 * Initialize fattr with "cf_nlink = 1" in cifs_dir_info_to_fattr. Without this, st_nlink for directory
 * will be zero in cifs filesystem.
 */

/**
 * Fix: DSM 4.1 #37011
 * Dsc: It will show "Data will be lost" while ENOSPC in some
 * environment (delayed allocation reserved blocks underflow in
 * file copy stress). This patch is fixed by Brian Foster, but
 * it is not applyed on linux. We should clean it up on next
 * kernel merge.
 */

/**
 * Fix: DSM 4.1 #38015
 * Dsc: Don't use rcu-walk before the bug "caseless stat
 * deadlock" solved.
 */

/******************************* Crypto. ********************************************/

/**
 * Dsc: Patches from freescale BSP
 */

/**
 * Dsc: Talitos channel management has bug on it. This fix that problem.
 *      We let XOR engine can only use one channel. And others can free to select
 */

/**
 * Dsc: Some 8533E board cannot pass boot up crypto self test.
 *      It makes me curious but we cannot reproduce it in our lab.
 *      So i just workaround it. I had tested this in customer machine. Support #54591, #55370
 *
 *      Please do not porting this, kernel has a new way to disable selftest.
 *	    Just enable it rather than use this define in newer kernel.
 */

/**
 * Dsc: Let ocf framework use linux crypto api to offload task
 *      to hw engine for those crypto driver don't implement ocf
 *      support.
 */

/*
 * combine ecryptfs and ocf framework.
 * Only open this function when OCF framework is used.
*/

/**
 * Dsc: convert blkcipher to ablkcipher , let ecrpytfs can use
 *      both hardware accelerate and software.
 */

/**
 * Dsc: This fix the memory leak issues in ocf_20100325 when we only use
 *      cryptosoft engine.
 */

/**
 * Dsc: Performance tuning for mv628x.
 */
#if defined(CONFIG_MV_XOR_MEMCOPY) && (defined(MY_DEF_HERE) || defined(MY_ABC_HERE))
#define SYNO_MV_PERF
#endif /* CONFIG_MV_XOR_MEMCOPY && (MY_DEF_HERE || MY_ABC_HERE) */

/******************************* Misc. ********************************************/
/**
 * Fix:
 * Dsc: Slow down hotplug event to prevent oom when plug a drive which has huge partitions
 */
#ifdef CONFIG_SYNO_MV88F6281_USBSTATION
#define SYNO_SLOW_DOWN_UEVENT
#endif

/**
 * Fix: #16748
 * Dsc: CPU speed adjusting while hibernation
 */

/**
 * Fix: DSM #111678
 * Dsc: Fix CVE-2017-0861: Use-after-free vulnerability in the snd_pcm_info function
 */

/******************************* MV BSP ********************************************/
/**
 * Fix: No Bug Entry
 * Dsc: Marvell's default BSP is used for 2.6.22, but we use 2.6.24.
 *      This define use to indentify what we porting.
 *      Because there had a little difference betwenn these two major version linux.
 * IN : mvLinuxIalHt.c, mvLinuxIalLib.c, mvLinuxIalSmart.c, thor/linux/linux_helper.c, thor/linux/linux_sense.c, ahci.c
 */

/**
 * Fix: No Bug Entry
 * Dsc: We use 2.6.32, modify Marvell's BSP related code (defaule is 2.6.22)
 *      This define use to indentify what we porting.
 *      Because there had a little difference betwenn these two major version linux.
 * IN : drivers/ata/mvSata_4_2_2/mvLinuxIalLib.c, drivers/ata/mvSata_4_2_2/mvLinuxIalHt.c,
 *      drivers/ata/mvSata_4_2_2/mvLinuxIalSmart.c
 */

/**
 * Fix: No Bug Entry
 * Dsc: Make mvSata driver accept CHECK_POWER ATA command
 *      (via Marvell SMART interface).
 * IN : mvStorageDev.h, mvLinuxIalSmart.c
 */
/**
 * Fix: DS 2.0 #8357
 * Dsc: implement error handle for Marvell mvSata
 */

#ifdef MY_ABC_HERE
#define MAX_CHANNEL_RETRY       2
#define CHANNEL_RETRY_INTERVAL  (3*HZ)

/**
 * Fix: DS 2.2 #10983
 * Dsc: let pmp device send cable event to user space
 */
#endif

/**
 * Fix : DS20 bug #5838, #8213
 *
 * Dsc : Some disk can't go through SRST status, so we reset it
 *      and give it one chance for detect disk eg. WD5001ABYS,
 *      Samsung HD103UJ, ST3500641AS
 *
 *      #8213: chip: Sunplus SataLink SPIF225A-HF021 (LC Power
 *      EH-35BE2 esata box)
 */

/**
 * Fix: No Bug entry
 * Dsc: In order to make LED static when disk present and blinking when
 *      disk active, we have to set the offset 0x104F0 bit 0-1
 *      to 0x00 and bit 2-3 to 1. See data sheet page 282 (Table
 *      232: GPIO port control register), this is for MV BSP driver
 *      MY_DEF_HERE is for open source driver
 * IN : mvSata.c
 */

/**
 * Fix DS2.0 #8115
 * Dsc: The port multiplier function of mvSata4.2.2 based driver
 *      is not good for 6281. They have some problem:
 *      1. Some devices can not be reconized by 6281.(Much like
 *         a chip bug)
 *         9410-HD-001
 *      2. Hotplug multiple devices in the same times is not
 *         well maintained.
 *      3. They could only recognize four disks at most.
 *
 * FIXME: I do not try this patch in 7042 with port multiplier.
 *        Because when i fix this problem, it is still not merge
 *        all the 7042 driver in the same package yet.
 */

/**
 * Fix: DS 2.0 #5942
 * Dsc: Let 2.6.24/15 kernel could use the sil-3726 PM , port
 *      from 2.6.15/drivers/scsi/mv7042/mvIALCommonUtils.c. I
 *      think it may be a version control miss in marvell.
 * IN : mvIALCommonUtils.c
 */

/**
 * Fix: DS 2.0 #7123
 * Dsc: Fix Marvell driver's bug. enlarge the link fault
 *      tolerance
 * Test Disks Info:
 *
 *  Product of China
 *  Device Model:     ST31000340NS
 *  Serial Number:    5QJ0W2RJ
 *  Firmware Version: SN75 -> AN05
 *  P/N:              9CA158-333
 *  User Capacity:    1,000,204,886,016 bytes
 *  ATA Version is:   8
 *  ATA Standard is:  ATA-8-ACS revision 4
 *
 *  Product of Thailand
 *  Device Model:     ST31000340NS
 *  Serial Number:    9QJ1MGC4
 *  Firmware Version: SN05 -> AN05
 *  P/N:              9CA158-303
 *  User Capacity:    1,000,204,886,016 bytes
 *  ATA Version is:   8
 *  ATA Standard is:  ATA-8-ACS revision 4
 *
 *  Product of Thailand
 *  Device Model:     ST31000340NS
 *  Serial Number:    9QJ1J81V
 *  Firmware Version: SN05 -> AN05
 *  P/N:              9CA158-303
 *  User Capacity:    1,000,204,886,016 bytes
 *  ATA Version is:   8
 *  ATA Standard is:  ATA-8-ACS revision 4
 *
 *  Product of Thailand
 *  Device Model:     ST31000340NS
 *  Serial Number:    9QJ1ME10
 *  Firmware Version: SN05 -> AN05
 *  P/N:              9CA158-303
 *  User Capacity:    1,000,203,804,160 bytes
 *  ATA Version is:   8
 *  ATA Standard is:  ATA-8-ACS revision 4
 *
 * IN : mvSata.c
 */

#include <uapi/linux/syno.h>

/**
* Fix: DSM #50678,#50679
* Dsc: Silent rpc svc_printk messages, it refers to kernel commit 624ab464 to silent rpc debug messages.
* So if we update to kernel version v3.9-rc1, this defination can be removed.
*/

/**
 * Fix: DSM #45988 ipv6+NFSv4 can not pass clientid confirm
 * Dsc: Function rpc_cmp_addr(__rpc_cmp_addr6) will compare ipv6 scope id on kernel version 3.2.
 * However, the copy function __rpc_copy_addr6 do not copy scope id,
 * So the client can not pass clientid confirm cause server think client's address is not match.
 * Copy scope id on function __rpc_copy_addr6 to solve this issue
 * This define can be removed after apply patch 155a345 on linux kernel version 3.9
 *
 * IN: include/linux/sunrpc/clnt.h
 */

/**
 *Fix: DSM #16952
 *Dsc: Modifies the MPP setting. This is used to switch
 *     the HDD leds off on DS211+
 */

/**
 * Fix: DSM #19665
 * Dsc: Add ECC notification support
 *		When ECC ram detect 1 bit error, it will correct this error automatically,
 *		but we still need a notification scheme let user know the error just occurred.
 */

#ifdef MY_ABC_HERE
/**
 * Fix: DSM #23148
 * Dsc: Make iSCSI device nodes to have prefix "ids"
 */
#define SYNO_ISCSI_DEVICE_INDEX    (26 + 25 * 26)    // start from za

#ifdef MY_DEF_HERE
/* Micro SD is mapped to USB "4-4" */
#define SYNO_INTERNAL_MICROSD_NAME "4-4"
#endif /* MY_DEF_HERE */
#endif /* MY_DEF_HERE */

/**
 * Fix: DSM #28156
 * Dsc: Upgrade compat-wireless v3.1.1
 *		New compat-wireless MAC80211 module need atomic64_t support,
 *		bug the config in linux-2.6.32 didn't add the atomic64 software emu
 *		in ARM arch.
 */

/**
 * Fix: DSM #35518
 * Dsc: The driver/base/core.c had been refactored. This causes that in sysfs, the symbolic link to block device dissappears.
 * For example: in /sys/class/scsi_disk/1:0:0:0/device/, there should be a symbolic link named as block:sas1.
 * However, this symbolic link dissappears in linux-3.x, and in linux-2.6.32, it preserves.
 * Because in SAS model, we relay on this structure to parse our disk topology, we need it back.
 * After checking the kernel git log & linux-2.6.32 related codes, this symbolic link could preserve here without affecting other functions.
 * It reverts commit 39aba963d937edb20db7d9d93e6dda5d2adfdcdd partially
 */

/**
 * Fix: DSM #35201
 * Dsc: if DS has Eunit power ctrl pin, Eunit supports zero watt sleep
 *
 */

/**
 * Fix: DSM #42868 - add DS114 model
 * Dsc: DS114 has turn EHCI hcd off and it will cause ehci init
 * hang. We need a kernel parameter to skip driver registering
 */

/*
 * Fix: DSM #47001
 * Dsc: Force set qoirq esata port to sata 1.5G
 */

/**
 * Fix bugs [DSM] #46499, #26543, #26544
 * When upload a file with icon from mac to ds, and the destination on ds is a cifs mount point of
 * another mac share, the upload action fail in mac os 10.6.2, or lose ea in mac os 10.6.8/
 * The reason is that icon is store in 'icon\r', and '\r' will be conver by mac.
 * Therefore we change '/r' (0x0d) to (0xf00d).
 */

/**
 * Fix <DSM> #49684, #49685
 * Linux-3.x uses cifs_strtoUCS to deal with log on, which is different with linux-2.6.x.
 * It causes LOGON_FAILURE when password contains '/'.
 */

/**
 * Fix <DSM> #54017
 * Dsd: Solve an issue that bdevname may assess released block_device and get Kernel OOPS
 * It happens when the block_device is released by ulock_rdev()
 */

/**
 * Fix <DSM> #65257
 *
 * Remove error log when receiving RA
 */

/**
 * Fix: DS 2.0 #5791, LKP2.6.32 #71
 * Dsc: Let raid read/write error can report to user through
 *      scemd and synobios
 * IN : synobios.c, md.c raid0.c, md.h, raid1.c, raid1.h
 */

/**
 * Fix: DSM #23923, DSM #54272
 * Dsc: Report LVM or MD sector which be auto remapped
 *      through scemd and synobios
 * IN : synobios.c
 */

/**
 * Fix: #59974
 * Dsc: Turn on/off the leds of the igb NIC
 */

#endif /* __SYNO_H_ */
