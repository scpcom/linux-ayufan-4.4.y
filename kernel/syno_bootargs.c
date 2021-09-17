// Copyright (c) 2003-2015 Synology Inc. All rights reserved.
#include <linux/kernel.h>
#include <linux/synolib.h>

#ifdef CONFIG_SYNO_DISK_INDEX_MAP
extern char gszDiskIdxMap[16];
#endif /* CONFIG_SYNO_DISK_INDEX_MAP */

#ifdef CONFIG_SYNO_HW_REVISION
extern char gszSynoHWRevision[];
#endif /* CONFIG_SYNO_HW_REVISION */

#ifdef CONFIG_SYNO_HW_VERSION
extern char gszSynoHWVersion[];
#endif /* CONFIG_SYNO_HW_VERSION */

#ifdef CONFIG_SYNO_INTERNAL_HD_NUM
extern long g_syno_hdd_powerup_seq;
#endif /* CONFIG_SYNO_INTERNAL_HD_NUM */

#ifdef CONFIG_SYNO_HDD_HOTPLUG
extern long g_hdd_hotplug;
#endif /* CONFIG_SYNO_HDD_HOTPLUG */

#ifdef CONFIG_SYNO_MAC_ADDRESS
extern unsigned char grgbLanMac[SYNO_MAC_MAX_NUMBER][16];
extern int giVenderFormatVersion;
extern char gszSkipVenderMacInterfaces[256];
#endif /* CONFIG_SYNO_MAC_ADDRESS */

#ifdef CONFIG_SYNO_SERIAL
extern char gszSerialNum[32];
extern char gszCustomSerialNum[32];
#endif /* CONFIG_SYNO_SERIAL */

#ifdef CONFIG_SYNO_SATA_DISK_SEQ_REVERSE
extern char giDiskSeqReverse[8];
#endif /* CONFIG_SYNO_SATA_DISK_SEQ_REVERSE */

#ifdef CONFIG_SYNO_INTERNAL_NETIF_NUM
extern long g_internal_netif_num;
#endif /* CONFIG_SYNO_INTERNAL_NETIF_NUM*/

#ifdef CONFIG_SYNO_SATA_MV_LED
extern long g_sata_mv_led;
#endif /* CONFIG_SYNO_SATA_MV_LED */

#ifdef CONFIG_SYNO_FACTORY_USB_FAST_RESET
extern int gSynoFactoryUSBFastReset;
#endif /* CONFIG_SYNO_FACTORY_USB_FAST_RESET */

#ifdef CONFIG_SYNO_FACTORY_USB3_DISABLE
extern int gSynoFactoryUSB3Disable;
#endif /* CONFIG_SYNO_FACTORY_USB3_DISABLE */

#ifdef CONFIG_SYNO_MEM_MODE_INFO
extern int gSynoMemMode;
#endif /* CONFIG_SYNO_MEM_MODE_INFO */

#ifdef CONFIG_SYNO_SWITCH_NET_DEVICE_NAME
extern unsigned int gSwitchDev;
extern char gDevPCIName[CONFIG_SYNO_MAX_SWITCHABLE_NET_DEVICE][CONFIG_SYNO_NET_DEVICE_ENCODING_LENGTH];
#endif /* CONFIG_SYNO_SWITCH_NET_DEVICE_NAME */

#ifdef CONFIG_SYNO_SAS_DISK_NAME
extern long g_is_sas_model;
#endif /* CONFIG_SYNO_SAS_DISK_NAME */

#ifdef CONFIG_SYNO_DUAL_HEAD
extern int gSynoDualHead;
#endif /* CONFIG_SYNO_DUAL_HEAD */

#ifdef CONFIG_SYNO_SAS_RESERVATION_WRITE_CONFLICT_KERNEL_PANIC
extern int gSynoSASWriteConflictPanic;
#endif /* CONFIG_SYNO_SAS_RESERVATION_WRITE_CONFLICT_KERNEL_PANIC */

#ifdef CONFIG_SYNO_BOOT_SATA_DOM
extern int gSynoBootSATADOM;
#endif /* CONFIG_SYNO_BOOT_SATA_DOM */

#ifdef CONFIG_SYNO_AHCI_SWITCH
extern char g_ahci_switch;
#endif /* CONFIG_SYNO_AHCI_SWITCH */

#ifdef CONFIG_SYNO_SATA_PORT_MAP
extern char gszSataPortMap[8];
#endif /* CONFIG_SYNO_SATA_PORT_MAP */

#ifdef CONFIG_SYNO_FIXED_DISK_NAME_MV14XX
extern char gszDiskIdxMapMv14xx[8];
#endif /* CONFIG_SYNO_FIXED_DISK_NAME_MV14XX */

#ifdef CONFIG_SYNO_CASTRATED_XHC
extern char gSynoCastratedXhcAddr[CONFIG_SYNO_NUM_CASTRATED_XHC][13];
extern unsigned int gSynoCastratedXhcPortBitmap[CONFIG_SYNO_NUM_CASTRATED_XHC];
#endif /* CONFIG_SYNO_CASTRATED_XHC */

#ifdef CONFIG_SYNO_USB_VBUS_GPIO_CONTROL
extern char gSynoUsbVbusHostAddr[CONFIG_SYNO_USB_VBUS_NUM_GPIO][13];
extern int gSynoUsbVbusPort[CONFIG_SYNO_USB_VBUS_NUM_GPIO];
extern unsigned gSynoUsbVbusGpp[CONFIG_SYNO_USB_VBUS_NUM_GPIO];
#endif /* CONFIG_SYNO_USB_VBUS_GPIO_CONTROL */

#ifdef CONFIG_SYNO_DISK_INDEX_MAP
static int __init early_disk_idx_map(char *p)
{
	snprintf(gszDiskIdxMap, sizeof(gszDiskIdxMap), "%s", p);

	if('\0' != gszDiskIdxMap[0]) {
		printk("Disk Index Map: %s\n", gszDiskIdxMap);
	}

	return 1;
}
__setup("DiskIdxMap=", early_disk_idx_map);
#endif /* CONFIG_SYNO_DISK_INDEX_MAP */

#ifdef CONFIG_SYNO_HW_REVISION
static int __init early_hw_revision(char *p)
{
       snprintf(gszSynoHWRevision, 4, "%s", p);

       printk("Synology Hardware Revision: %s\n", gszSynoHWRevision);

       return 1;
}
__setup("rev=", early_hw_revision);
#endif /* CONFIG_SYNO_HW_REVISION */

#ifdef CONFIG_SYNO_HW_VERSION
static int __init early_hw_version(char *p)
{
	char *szPtr;

	snprintf(gszSynoHWVersion, 16, "%s", p);

	szPtr = gszSynoHWVersion;
	while ((*szPtr != ' ') && (*szPtr != '\t') && (*szPtr != '\0')) {
		szPtr++;
	}
	*szPtr = 0;
	strcat(gszSynoHWVersion, "-j");

	printk("Synology Hardware Version: %s\n", gszSynoHWVersion);

	return 1;
}
__setup("syno_hw_version=", early_hw_version);
#endif /* CONFIG_SYNO_HW_VERSION */

#ifdef CONFIG_SYNO_INTERNAL_HD_NUM
/* It is recommanded to use syno_hdd_powerup_seq instead of ihd_num.
 * Because the actual usage of the variable is represented the powerup seq,
 * the syno_hdd_powerup_seq is designed to replace ihd_num.
 * */
static int __init early_internal_hd_num(char *p)
{
        g_syno_hdd_powerup_seq = simple_strtol(p, NULL, 10);

        printk("Internal HD num: %d\n", (int)g_syno_hdd_powerup_seq);

        return 1;
}
__setup("ihd_num=", early_internal_hd_num);

static int __init syno_hdd_powerup_seq(char *p)
{
        g_syno_hdd_powerup_seq = simple_strtol(p, NULL, 10);

        printk("Power on seq num: %d\n", (int)g_syno_hdd_powerup_seq);

        return 1;
}
__setup("syno_hdd_powerup_seq=", syno_hdd_powerup_seq);
#endif /* CONFIG_SYNO_INTERNAL_HD_NUM */

#ifdef CONFIG_SYNO_HDD_HOTPLUG
/* It is recommanded to use enable_hdd_hotplug instead of HddHotplug.
 * Beacuse the bootarg is referred to a bool variable,
 * the enable_hdd_hotplug is designed to replace HddHotplug.
 * */
static int __init early_hdd_hotplug(char *p)
{
	g_hdd_hotplug = simple_strtol(p, NULL, 10);

	if ( g_hdd_hotplug > 0 ) {
		printk("Support HDD Hotplug.\n");
	}

	return 1;
}
__setup("HddHotplug=", early_hdd_hotplug);

static int __init enable_hdd_hotplug(char *p)
{
	g_hdd_hotplug = simple_strtol(p, NULL, 10);

	if ( g_hdd_hotplug > 0 ) {
		printk("Support HDD Hotplug.\n");
	}

	return 1;
}
__setup("enable_hdd_hotplug=", enable_hdd_hotplug);
#endif /* CONFIG_SYNO_HDD_HOTPLUG */

#ifdef CONFIG_SYNO_MAC_ADDRESS
static int __init early_mac1(char *p)
{
	snprintf(grgbLanMac[0], sizeof(grgbLanMac[0]), "%s", p);

	printk("Mac1: %s\n", grgbLanMac[0]);

	return 1;
}
__setup("mac1=", early_mac1);

static int __init early_mac2(char *p)
{
	snprintf(grgbLanMac[1], sizeof(grgbLanMac[1]), "%s", p);

	printk("Mac2: %s\n", grgbLanMac[1]);

	return 1;
}
__setup("mac2=", early_mac2);

static int __init early_mac3(char *p)
{
	snprintf(grgbLanMac[2], sizeof(grgbLanMac[2]), "%s", p);

	printk("Mac3: %s\n", grgbLanMac[2]);

	return 1;
}
__setup("mac3=", early_mac3);

static int __init early_mac4(char *p)
{
	snprintf(grgbLanMac[3], sizeof(grgbLanMac[3]), "%s", p);

	printk("Mac4: %s\n", grgbLanMac[3]);

	return 1;
}
__setup("mac4=", early_mac4);

static int __init early_macs(char *p)
{
	int iMacCount = 0;
	char *pBegin = p;
	char *pEnd = strstr(pBegin, ",");

	while (NULL != pEnd && SYNO_MAC_MAX_NUMBER > iMacCount) {
		*pEnd = '\0';
		snprintf(grgbLanMac[iMacCount], sizeof(grgbLanMac[iMacCount]), "%s", pBegin);
		pBegin = pEnd + 1;
		pEnd = strstr(pBegin, ",");
		iMacCount++;
	}

	if ('\0' != *pBegin && SYNO_MAC_MAX_NUMBER > iMacCount) {
		snprintf(grgbLanMac[iMacCount], sizeof(grgbLanMac[iMacCount]), "%s", pBegin);
	}

	return 1;
}
__setup("macs=", early_macs);

static int __init early_vender_format_version(char *p)
{
	giVenderFormatVersion = simple_strtol(p, NULL, 10);

	printk("Vender format version: %d\n", giVenderFormatVersion);

	return 1;
}
__setup("vender_format_version=", early_vender_format_version);

static int __init early_skip_vender_mac_interfaces(char *p)
{
	snprintf(gszSkipVenderMacInterfaces, sizeof(gszSkipVenderMacInterfaces), "%s", p);

	printk("Skip vender mac interfaces: %s\n", gszSkipVenderMacInterfaces);

	return 1;
}
__setup("skip_vender_mac_interfaces=", early_skip_vender_mac_interfaces);
#endif /* CONFIG_SYNO_MAC_ADDRESS */

#ifdef CONFIG_SYNO_SERIAL
static int __init early_sn(char *p)
{
	snprintf(gszSerialNum, sizeof(gszSerialNum), "%s", p);
	printk("Serial Number: %s\n", gszSerialNum);
	return 1;
}
__setup("sn=", early_sn);

static int __init early_custom_sn(char *p)
{
	snprintf(gszCustomSerialNum, sizeof(gszCustomSerialNum), "%s", p);
	printk("Custom Serial Number: %s\n", gszCustomSerialNum);
	return 1;
}
__setup("custom_sn=", early_custom_sn);
#endif /* CONFIG_SYNO_SERIAL */

#ifdef CONFIG_SYNO_FACTORY_USB_FAST_RESET
static int __init early_factory_usb_fast_reset(char *p)
{
	gSynoFactoryUSBFastReset = simple_strtol(p, NULL, 10);

	printk("Factory USB Fast Reset: %d\n", (int)gSynoFactoryUSBFastReset);

	return 1;
}
__setup("syno_usb_fast_reset=", early_factory_usb_fast_reset);
#endif /* CONFIG_SYNO_FACTORY_USB_FAST_RESET */

#ifdef CONFIG_SYNO_FACTORY_USB3_DISABLE
static int __init early_factory_usb3_disable(char *p)
{
	gSynoFactoryUSB3Disable = simple_strtol(p, NULL, 10);

	printk("Factory USB3 Disable: %d\n", (int)gSynoFactoryUSB3Disable);

	return 1;
}
__setup("syno_disable_usb3=", early_factory_usb3_disable);
#endif /* CONFIG_SYNO_FACTORY_USB3_DISABLE */

#ifdef CONFIG_SYNO_SATA_DISK_SEQ_REVERSE
static int __init early_disk_seq_reserve(char *p)
{
	snprintf(giDiskSeqReverse, sizeof(giDiskSeqReverse), "%s", p);

	if('\0' != giDiskSeqReverse[0]) {
		printk("Disk Sequence Reverse: %s\n", giDiskSeqReverse);
	}

	return 1;
}
__setup("DiskSeqReverse=", early_disk_seq_reserve);
#endif /* CONFIG_SYNO_SATA_DISK_SEQ_REVERSE */

#ifdef  CONFIG_SYNO_INTERNAL_NETIF_NUM
static int __init early_internal_netif_num(char *p)
{
	g_internal_netif_num = simple_strtol(p, NULL, 10);

	if ( g_internal_netif_num >= 0 ) {
		printk("Internal netif num: %d\n", (int)g_internal_netif_num);
	}

	return 1;
}
__setup("netif_num=", early_internal_netif_num);
#endif /* CONFIG_SYNO_INTERNAL_NETIF_NUM */

#ifdef CONFIG_SYNO_SATA_MV_LED
static int __init early_sataled_special(char *p)
{
	g_sata_mv_led = simple_strtol(p, NULL, 10);

	if ( g_sata_mv_led >= 0 ) {
		printk("Special Sata LEDs.\n");
	}

	return 1;
}
__setup("SataLedSpecial=", early_sataled_special);
#endif /* CONFIG_SYNO_SATA_MV_LED */

#ifdef CONFIG_SYNO_MEM_MODE_INFO
static int __init early_mem_mode(int *p)
{
	gSynoMemMode = simple_strtol(p, NULL, 10);

	printk("SYNO Transcoding Memory Mode: %d\n", (int)gSynoMemMode);

	return 1;
}
__setup("syno_mem_mode=", early_mem_mode);
#endif /* CONFIG_SYNO_MEM_MODE_INFO */

#ifdef CONFIG_SYNO_SWITCH_NET_DEVICE_NAME
static int __init early_netif_seq(char *p)
{
	int len;
	int netDevCount;

	// no net device switch required
	if ((NULL == p) || (0 == (len = strlen(p)))) {
		return 1;
	}

	/**
	 *	We change the way that we represent the net device name is due to a truth that
	 *	when a PCIE extension card is plugged in, the pcie name will change
	 *	So we give up the pci-name as our matching condition, we use NIC up sequence instead.
	 *	Because the NIC layout is fixed on our board, we the NIC up sequence won't change.
	 *	And according to this sequence, we assign the device name to NIC
	 *
	 *	Following codes are designed to compatible with bromolow/x64 which has already been produced.
	 *	Based on the truth that our bromolow/x64 has at least 2 internal lan so far (2011/5/24)
	 *	And 2 internal lan needs netif_seq whose length is 12
	 *	So we judge that if netif_seq is less than 12, then it should be new version of netif_seq
	 *	2411+ has 2 internal lans now so we use 12 as our boundary condition
	 */
	if (len <= CONFIG_SYNO_MAX_SWITCHABLE_NET_DEVICE) {
		netDevCount = len;
		for(gSwitchDev = 0 ; gSwitchDev < netDevCount && gSwitchDev < CONFIG_SYNO_MAX_SWITCHABLE_NET_DEVICE ; gSwitchDev++) {
			gDevPCIName[gSwitchDev][0] = *p++;
		}
		return 1;
	}

	netDevCount = len/CONFIG_SYNO_NET_DEVICE_ENCODING_LENGTH;
	if (0 == netDevCount) {
		return 1;
	}

	for(gSwitchDev = 0 ; gSwitchDev < netDevCount && gSwitchDev < CONFIG_SYNO_MAX_SWITCHABLE_NET_DEVICE ; gSwitchDev++) {
		// the format of netif_seq string is device seq (1 character) + device pci name (last 5 characters only)
		memcpy(gDevPCIName[gSwitchDev], p, CONFIG_SYNO_NET_DEVICE_ENCODING_LENGTH);
		p += CONFIG_SYNO_NET_DEVICE_ENCODING_LENGTH;
	}
	return 1;
}

__setup("netif_seq=",early_netif_seq);
#endif /* CONFIG_SYNO_SWITCH_NET_DEVICE_NAME */

#ifdef CONFIG_SYNO_FIXED_DISK_NAME_MV14XX
static int __init early_disk_idx_map_mv14xx(char *p)
{
	snprintf(gszDiskIdxMapMv14xx, sizeof(gszDiskIdxMapMv14xx), "%s", p);

	if('\0' != gszDiskIdxMapMv14xx[0]) {
		printk("Disk Indx Map on MV14xx: %s\n", gszDiskIdxMapMv14xx);
	}

	return 1;
}
__setup("DiskIdxMapMv14xx=", early_disk_idx_map_mv14xx);
#endif /* CONFIG_SYNO_FIXED_DISK_NAME_MV14XX */

#ifdef CONFIG_SYNO_SATA_PORT_MAP
static int __init early_sataport_map(char *p)
{
	snprintf(gszSataPortMap, sizeof(gszSataPortMap), "%s", p);

	if(0 != gszSataPortMap[0]) {
		printk("Sata Port Map: %s\n", gszSataPortMap);
	}

	return 1;
}
__setup("SataPortMap=", early_sataport_map);
#endif /* CONFIG_SYNO_SATA_PORT_MAP */

#ifdef CONFIG_SYNO_SAS_DISK_NAME
static int __init early_SASmodel(char *p)
{
	g_is_sas_model = simple_strtol(p, NULL, 10);

	if (1 == g_is_sas_model) {
		printk("SAS model: %d\n", (int)g_is_sas_model);
	}

	return 1;
}
__setup("SASmodel=", early_SASmodel);
#endif /* CONFIG_SYNO_SAS_DISK_NAME */

#ifdef CONFIG_SYNO_DUAL_HEAD
static int __init early_dual_head(char *p)
{
	gSynoDualHead = simple_strtol(p, NULL, 10);
#ifdef CONFIG_SYNO_BOOT_SATA_DOM
	gSynoBootSATADOM = gSynoDualHead;
#endif /* CONFIG_SYNO_BOOT_SATA_DOM */
#ifdef CONFIG_SYNO_SAS_RESERVATION_WRITE_CONFLICT_KERNEL_PANIC
	gSynoSASWriteConflictPanic = gSynoDualHead;
#endif

	printk("Synology Dual Head: %d\n", gSynoDualHead);

	return 1;
}
__setup("dual_head=", early_dual_head);
#endif /* CONFIG_SYNO_DUAL_HEAD */

#ifdef CONFIG_SYNO_SAS_RESERVATION_WRITE_CONFLICT_KERNEL_PANIC
static int __init early_sas_reservation_write_conflict(char *p)
{
	gSynoSASWriteConflictPanic = simple_strtol(p, NULL, 10);

	printk("Let kernel panic if sas reservation write conflict: %d\n", gSynoSASWriteConflictPanic);

	return 1;
}
__setup("sas_reservation_write_conflict=", early_sas_reservation_write_conflict);
#endif /* CONFIG_SYNO_SAS_RESERVATION_WRITE_CONFLICT_KERNEL_PANIC */

#ifdef CONFIG_SYNO_BOOT_SATA_DOM
static int __init early_synoboot_satadom(char *p)
{
	gSynoBootSATADOM = simple_strtol(p, NULL, 10);

	printk("Synology boot device SATADOM: %d\n", gSynoBootSATADOM);

	return 1;
}
__setup("synoboot_satadom=", early_synoboot_satadom);
#endif /* CONFIG_SYNO_BOOT_SATA_DOM */

#ifdef  CONFIG_SYNO_AHCI_SWITCH
static int __init early_ahci_switch(char *p)
{
	g_ahci_switch = p[0];
	if ('0' == g_ahci_switch) {
		printk("AHCI: 0\n");
	} else {
		printk("AHCI: 1\n");
	}

	return 1;
}
__setup("ahci=", early_ahci_switch);
#endif /* CONFIG_SYNO_AHCI_SWITCH */

#ifdef CONFIG_SYNO_CASTRATED_XHC
static int __init early_castrated_xhc(char *p)
{
	int iCount = 0;
	char *pBegin = p;
	char *pEnd = strstr(pBegin, ",");
	char *pPortSep = NULL;

	while (iCount < CONFIG_SYNO_NUM_CASTRATED_XHC) {
		if(NULL != pEnd)
			*pEnd = '\0';
		pPortSep = strstr(pBegin, "@");
		if (pPortSep == NULL) {
			printk("Castrated xHC - Parameter format not correct\n");
			break;
		}
		*pPortSep = '\0';
		snprintf(gSynoCastratedXhcAddr[iCount],
				sizeof(gSynoCastratedXhcAddr[iCount]), "%s", pBegin);
		gSynoCastratedXhcPortBitmap[iCount] = simple_strtoul(pPortSep + 1, NULL,
				16);
		if (NULL == pEnd)
			break;
		pBegin = pEnd + 1;
		pEnd = strstr(pBegin, ",");
		iCount++;
	}

	return 1;
}
__setup("syno_castrated_xhc=", early_castrated_xhc);
#endif /* CONFIG_SYNO_CASTRATED_XHC */

#ifdef CONFIG_SYNO_USB_VBUS_GPIO_CONTROL
static int __init early_usb_vbus_gpio(char *p)
{
	int iCount = 0;
	char *pBegin = p;
	char *pEnd = strstr(pBegin, ",");
	char *pSeparator = NULL;
	int error = 0;

	printk("USB Vbus GPIO Control:\n");

	while (iCount < CONFIG_SYNO_USB_VBUS_NUM_GPIO) {
		if(NULL != pEnd)
			*pEnd = '\0';

		pSeparator = strstr(pBegin, "@");
		if (pSeparator == NULL) {
			printk("USB Vbus GPIO Control - Parameter format not correct\n");
			error = 1;
			break;
		}
		*pSeparator = '\0';
		gSynoUsbVbusGpp[iCount] = simple_strtoul(pBegin, NULL, 10);
		printk(" Gpp#%d", gSynoUsbVbusGpp[iCount]);

		pBegin = pSeparator + 1;
		pSeparator = strstr(pBegin, "@");
		if (pSeparator == NULL) {
			printk("\nUSB Vbus GPIO Control - Parameter format not correct\n");
			error = 1;
			break;
		}
		*pSeparator = '\0';
		snprintf(gSynoUsbVbusHostAddr[iCount],
				sizeof(gSynoUsbVbusHostAddr[iCount]), "%s", pBegin);
		printk(" - Host:%s", gSynoUsbVbusHostAddr[iCount]);

		gSynoUsbVbusPort[iCount] = simple_strtoul(pSeparator + 1, NULL, 10);
		printk(" - Port:%d", gSynoUsbVbusPort[iCount]);

		if (NULL == pEnd)
			break;

		pBegin = pEnd + 1;
		pEnd = strstr(pBegin, ",");
		iCount++;
	}

	if (error) {
		iCount = 0;
		while (iCount < CONFIG_SYNO_USB_VBUS_NUM_GPIO) {
			gSynoUsbVbusHostAddr[iCount][0] = '\0';
			gSynoUsbVbusGpp[iCount] = 0;
			gSynoUsbVbusPort[iCount] = 0;
			iCount++;
		}
	}

	return 1;
}
__setup("syno_usb_vbus_gpio=", early_usb_vbus_gpio);
#endif /* CONFIG_SYNO_USB_VBUS_GPIO_CONTROL */
