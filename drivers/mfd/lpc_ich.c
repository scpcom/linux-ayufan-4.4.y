#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  lpc_ich.c - LPC interface for Intel ICH
 *
 *  LPC bridge function of the Intel ICH contains many other
 *  functional units, such as Interrupt controllers, Timers,
 *  Power Management, System Management, GPIO, RTC, and LPC
 *  Configuration Registers.
 *
 *  This driver is derived from lpc_sch.

 *  Copyright (c) 2011 Extreme Engineering Solution, Inc.
 *  Author: Aaron Sierra <asierra@xes-inc.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  This driver supports the following I/O Controller hubs:
 *	(See the intel documentation on http://developer.intel.com.)
 *	document number 290655-003, 290677-014: 82801AA (ICH), 82801AB (ICHO)
 *	document number 290687-002, 298242-027: 82801BA (ICH2)
 *	document number 290733-003, 290739-013: 82801CA (ICH3-S)
 *	document number 290716-001, 290718-007: 82801CAM (ICH3-M)
 *	document number 290744-001, 290745-025: 82801DB (ICH4)
 *	document number 252337-001, 252663-008: 82801DBM (ICH4-M)
 *	document number 273599-001, 273645-002: 82801E (C-ICH)
 *	document number 252516-001, 252517-028: 82801EB (ICH5), 82801ER (ICH5R)
 *	document number 300641-004, 300884-013: 6300ESB
 *	document number 301473-002, 301474-026: 82801F (ICH6)
 *	document number 313082-001, 313075-006: 631xESB, 632xESB
 *	document number 307013-003, 307014-024: 82801G (ICH7)
 *	document number 322896-001, 322897-001: NM10
 *	document number 313056-003, 313057-017: 82801H (ICH8)
 *	document number 316972-004, 316973-012: 82801I (ICH9)
 *	document number 319973-002, 319974-002: 82801J (ICH10)
 *	document number 322169-001, 322170-003: 5 Series, 3400 Series (PCH)
 *	document number 320066-003, 320257-008: EP80597 (IICH)
 *	document number 324645-001, 324646-001: Cougar Point (CPT)
 *	document number TBD : Patsburg (PBG)
 *	document number TBD : DH89xxCC
 *	document number TBD : Panther Point
 *	document number TBD : Lynx Point
 */
 
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/acpi.h>
#include <linux/pci.h>
#include <linux/mfd/core.h>
#include <linux/mfd/lpc_ich.h>

#define ACPIBASE		0x40
#define ACPIBASE_GPE_OFF	0x28
#define ACPIBASE_GPE_END	0x2f
#define ACPICTRL		0x44

#define GPIOBASE		0x48
#define GPIOCTRL		0x4C

static int lpc_ich_acpi_save = -1;
static int lpc_ich_gpio_save = -1;

static struct resource gpio_ich_res[] = {
	/* GPIO */
	{
		.flags = IORESOURCE_IO,
	},
	/* ACPI - GPE0 */
	{
		.flags = IORESOURCE_IO,
	},
};

enum lpc_cells {
	LPC_GPIO = 0,
};

static struct mfd_cell lpc_ich_cells[] = {
	[LPC_GPIO] = {
		.name = "gpio_ich",
		.num_resources = ARRAY_SIZE(gpio_ich_res),
		.resources = gpio_ich_res,
		.ignore_resource_conflicts = true,
	},
};

/* chipset related info */
enum lpc_chipsets {
	LPC_ICH = 0,	/* ICH */
	LPC_ICH0,	/* ICH0 */
	LPC_ICH2,	/* ICH2 */
	LPC_ICH2M,	/* ICH2-M */
	LPC_ICH3,	/* ICH3-S */
	LPC_ICH3M,	/* ICH3-M */
	LPC_ICH4,	/* ICH4 */
	LPC_ICH4M,	/* ICH4-M */
	LPC_CICH,	/* C-ICH */
	LPC_ICH5,	/* ICH5 & ICH5R */
	LPC_6300ESB,	/* 6300ESB */
	LPC_ICH6,	/* ICH6 & ICH6R */
	LPC_ICH6M,	/* ICH6-M */
	LPC_ICH6W,	/* ICH6W & ICH6RW */
	LPC_631XESB,	/* 631xESB/632xESB */
	LPC_ICH7,	/* ICH7 & ICH7R */
	LPC_ICH7DH,	/* ICH7DH */
	LPC_ICH7M,	/* ICH7-M & ICH7-U */
	LPC_ICH7MDH,	/* ICH7-M DH */
	LPC_NM10,	/* NM10 */
	LPC_ICH8,	/* ICH8 & ICH8R */
	LPC_ICH8DH,	/* ICH8DH */
	LPC_ICH8DO,	/* ICH8DO */
	LPC_ICH8M,	/* ICH8M */
	LPC_ICH8ME,	/* ICH8M-E */
	LPC_ICH9,	/* ICH9 */
	LPC_ICH9R,	/* ICH9R */
	LPC_ICH9DH,	/* ICH9DH */
	LPC_ICH9DO,	/* ICH9DO */
	LPC_ICH9M,	/* ICH9M */
	LPC_ICH9ME,	/* ICH9M-E */
	LPC_ICH10,	/* ICH10 */
	LPC_ICH10R,	/* ICH10R */
	LPC_ICH10D,	/* ICH10D */
	LPC_ICH10DO,	/* ICH10DO */
	LPC_PCH,	/* PCH Desktop Full Featured */
	LPC_PCHM,	/* PCH Mobile Full Featured */
	LPC_P55,	/* P55 */
	LPC_PM55,	/* PM55 */
	LPC_H55,	/* H55 */
	LPC_QM57,	/* QM57 */
	LPC_H57,	/* H57 */
	LPC_HM55,	/* HM55 */
	LPC_Q57,	/* Q57 */
	LPC_HM57,	/* HM57 */
	LPC_PCHMSFF,	/* PCH Mobile SFF Full Featured */
	LPC_QS57,	/* QS57 */
	LPC_3400,	/* 3400 */
	LPC_3420,	/* 3420 */
	LPC_3450,	/* 3450 */
	LPC_EP80579,	/* EP80579 */
	LPC_CPT,	/* Cougar Point */
	LPC_CPTD,	/* Cougar Point Desktop */
	LPC_CPTM,	/* Cougar Point Mobile */
	LPC_PBG,	/* Patsburg */
	LPC_DH89XXCC,	/* DH89xxCC */
	LPC_PPT,	/* Panther Point */
	LPC_LPT,	/* Lynx Point */
#ifdef CONFIG_SYNO_AVOTON
	LPC_AVN,	/* Avoton SoC */
#endif
};

struct lpc_ich_info lpc_chipset_info[] __devinitdata = {
	[LPC_ICH] = {
		.name = "ICH",
	},
	[LPC_ICH0] = {
		.name = "ICH0",
	},
	[LPC_ICH2] = {
		.name = "ICH2",
	},
	[LPC_ICH2M] = {
		.name = "ICH2-M",
	},
	[LPC_ICH3] = {
		.name = "ICH3-S",
	},
	[LPC_ICH3M] = {
		.name = "ICH3-M",
	},
	[LPC_ICH4] = {
		.name = "ICH4",
	},
	[LPC_ICH4M] = {
		.name = "ICH4-M",
	},
	[LPC_CICH] = {
		.name = "C-ICH",
	},
	[LPC_ICH5] = {
		.name = "ICH5 or ICH5R",
	},
	[LPC_6300ESB] = {
		.name = "6300ESB",
	},
	[LPC_ICH6] = {
		.name = "ICH6 or ICH6R",
		.gpio_version = ICH_V6_GPIO,
	},
	[LPC_ICH6M] = {
		.name = "ICH6-M",
		.gpio_version = ICH_V6_GPIO,
	},
	[LPC_ICH6W] = {
		.name = "ICH6W or ICH6RW",
		.gpio_version = ICH_V6_GPIO,
	},
	[LPC_631XESB] = {
		.name = "631xESB/632xESB",
		.gpio_version = ICH_V6_GPIO,
	},
	[LPC_ICH7] = {
		.name = "ICH7 or ICH7R",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_ICH7DH] = {
		.name = "ICH7DH",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_ICH7M] = {
		.name = "ICH7-M or ICH7-U",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_ICH7MDH] = {
		.name = "ICH7-M DH",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_NM10] = {
		.name = "NM10",
	},
	[LPC_ICH8] = {
		.name = "ICH8 or ICH8R",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_ICH8DH] = {
		.name = "ICH8DH",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_ICH8DO] = {
		.name = "ICH8DO",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_ICH8M] = {
		.name = "ICH8M",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_ICH8ME] = {
		.name = "ICH8M-E",
		.gpio_version = ICH_V7_GPIO,
	},
	[LPC_ICH9] = {
		.name = "ICH9",
		.gpio_version = ICH_V9_GPIO,
	},
	[LPC_ICH9R] = {
		.name = "ICH9R",
		.gpio_version = ICH_V9_GPIO,
	},
	[LPC_ICH9DH] = {
		.name = "ICH9DH",
		.gpio_version = ICH_V9_GPIO,
	},
	[LPC_ICH9DO] = {
		.name = "ICH9DO",
		.gpio_version = ICH_V9_GPIO,
	},
	[LPC_ICH9M] = {
		.name = "ICH9M",
		.gpio_version = ICH_V9_GPIO,
	},
	[LPC_ICH9ME] = {
		.name = "ICH9M-E",
		.gpio_version = ICH_V9_GPIO,
	},
	[LPC_ICH10] = {
		.name = "ICH10",
		.gpio_version = ICH_V10CONS_GPIO,
	},
	[LPC_ICH10R] = {
		.name = "ICH10R",
		.gpio_version = ICH_V10CONS_GPIO,
	},
	[LPC_ICH10D] = {
		.name = "ICH10D",
		.gpio_version = ICH_V10CORP_GPIO,
	},
	[LPC_ICH10DO] = {
		.name = "ICH10DO",
		.gpio_version = ICH_V10CORP_GPIO,
	},
	[LPC_PCH] = {
		.name = "PCH Desktop Full Featured",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_PCHM] = {
		.name = "PCH Mobile Full Featured",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_P55] = {
		.name = "P55",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_PM55] = {
		.name = "PM55",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_H55] = {
		.name = "H55",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_QM57] = {
		.name = "QM57",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_H57] = {
		.name = "H57",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_HM55] = {
		.name = "HM55",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_Q57] = {
		.name = "Q57",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_HM57] = {
		.name = "HM57",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_PCHMSFF] = {
		.name = "PCH Mobile SFF Full Featured",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_QS57] = {
		.name = "QS57",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_3400] = {
		.name = "3400",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_3420] = {
		.name = "3420",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_3450] = {
		.name = "3450",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_EP80579] = {
		.name = "EP80579",
	},
	[LPC_CPT] = {
		.name = "Cougar Point",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_CPTD] = {
		.name = "Cougar Point Desktop",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_CPTM] = {
		.name = "Cougar Point Mobile",
		.gpio_version = ICH_V5_GPIO,
	},
	[LPC_PBG] = {
		.name = "Patsburg",
	},
	[LPC_DH89XXCC] = {
		.name = "DH89xxCC",
	},
	[LPC_PPT] = {
		.name = "Panther Point",
	},
	[LPC_LPT] = {
		.name = "Lynx Point",
	},
#ifdef CONFIG_SYNO_AVOTON
	[LPC_AVN] = {
		.name = "Avoton SoC",
	},
#endif
};

/*
 * This data only exists for exporting the supported PCI ids
 * via MODULE_DEVICE_TABLE.  We do not actually register a
 * pci_driver, because the I/O Controller Hub has also other
 * functions that probably will be registered by other drivers.
 */
static DEFINE_PCI_DEVICE_TABLE(lpc_ich_ids) = {
	{ PCI_VDEVICE(INTEL, 0x2410), LPC_ICH},
	{ PCI_VDEVICE(INTEL, 0x2420), LPC_ICH0},
	{ PCI_VDEVICE(INTEL, 0x2440), LPC_ICH2},
	{ PCI_VDEVICE(INTEL, 0x244c), LPC_ICH2M},
	{ PCI_VDEVICE(INTEL, 0x2480), LPC_ICH3},
	{ PCI_VDEVICE(INTEL, 0x248c), LPC_ICH3M},
	{ PCI_VDEVICE(INTEL, 0x24c0), LPC_ICH4},
	{ PCI_VDEVICE(INTEL, 0x24cc), LPC_ICH4M},
	{ PCI_VDEVICE(INTEL, 0x2450), LPC_CICH},
	{ PCI_VDEVICE(INTEL, 0x24d0), LPC_ICH5},
	{ PCI_VDEVICE(INTEL, 0x25a1), LPC_6300ESB},
	{ PCI_VDEVICE(INTEL, 0x2640), LPC_ICH6},
	{ PCI_VDEVICE(INTEL, 0x2641), LPC_ICH6M},
	{ PCI_VDEVICE(INTEL, 0x2642), LPC_ICH6W},
	{ PCI_VDEVICE(INTEL, 0x2670), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2671), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2672), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2673), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2674), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2675), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2676), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2677), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2678), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x2679), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x267a), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x267b), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x267c), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x267d), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x267e), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x267f), LPC_631XESB},
	{ PCI_VDEVICE(INTEL, 0x27b8), LPC_ICH7},
	{ PCI_VDEVICE(INTEL, 0x27b0), LPC_ICH7DH},
	{ PCI_VDEVICE(INTEL, 0x27b9), LPC_ICH7M},
	{ PCI_VDEVICE(INTEL, 0x27bd), LPC_ICH7MDH},
	{ PCI_VDEVICE(INTEL, 0x27bc), LPC_NM10},
	{ PCI_VDEVICE(INTEL, 0x2810), LPC_ICH8},
	{ PCI_VDEVICE(INTEL, 0x2812), LPC_ICH8DH},
	{ PCI_VDEVICE(INTEL, 0x2814), LPC_ICH8DO},
	{ PCI_VDEVICE(INTEL, 0x2815), LPC_ICH8M},
	{ PCI_VDEVICE(INTEL, 0x2811), LPC_ICH8ME},
	{ PCI_VDEVICE(INTEL, 0x2918), LPC_ICH9},
	{ PCI_VDEVICE(INTEL, 0x2916), LPC_ICH9R},
	{ PCI_VDEVICE(INTEL, 0x2912), LPC_ICH9DH},
	{ PCI_VDEVICE(INTEL, 0x2914), LPC_ICH9DO},
	{ PCI_VDEVICE(INTEL, 0x2919), LPC_ICH9M},
	{ PCI_VDEVICE(INTEL, 0x2917), LPC_ICH9ME},
	{ PCI_VDEVICE(INTEL, 0x3a18), LPC_ICH10},
	{ PCI_VDEVICE(INTEL, 0x3a16), LPC_ICH10R},
	{ PCI_VDEVICE(INTEL, 0x3a1a), LPC_ICH10D},
	{ PCI_VDEVICE(INTEL, 0x3a14), LPC_ICH10DO},
	{ PCI_VDEVICE(INTEL, 0x3b00), LPC_PCH},
	{ PCI_VDEVICE(INTEL, 0x3b01), LPC_PCHM},
	{ PCI_VDEVICE(INTEL, 0x3b02), LPC_P55},
	{ PCI_VDEVICE(INTEL, 0x3b03), LPC_PM55},
	{ PCI_VDEVICE(INTEL, 0x3b06), LPC_H55},
	{ PCI_VDEVICE(INTEL, 0x3b07), LPC_QM57},
	{ PCI_VDEVICE(INTEL, 0x3b08), LPC_H57},
	{ PCI_VDEVICE(INTEL, 0x3b09), LPC_HM55},
	{ PCI_VDEVICE(INTEL, 0x3b0a), LPC_Q57},
	{ PCI_VDEVICE(INTEL, 0x3b0b), LPC_HM57},
	{ PCI_VDEVICE(INTEL, 0x3b0d), LPC_PCHMSFF},
	{ PCI_VDEVICE(INTEL, 0x3b0f), LPC_QS57},
	{ PCI_VDEVICE(INTEL, 0x3b12), LPC_3400},
	{ PCI_VDEVICE(INTEL, 0x3b14), LPC_3420},
	{ PCI_VDEVICE(INTEL, 0x3b16), LPC_3450},
	{ PCI_VDEVICE(INTEL, 0x5031), LPC_EP80579},
	{ PCI_VDEVICE(INTEL, 0x1c41), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c42), LPC_CPTD},
	{ PCI_VDEVICE(INTEL, 0x1c43), LPC_CPTM},
	{ PCI_VDEVICE(INTEL, 0x1c44), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c45), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c46), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c47), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c48), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c49), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c4a), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c4b), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c4c), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c4d), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c4e), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c4f), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c50), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c51), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c52), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c53), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c54), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c55), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c56), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c57), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c58), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c59), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c5a), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c5b), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c5c), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c5d), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c5e), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1c5f), LPC_CPT},
	{ PCI_VDEVICE(INTEL, 0x1d40), LPC_PBG},
	{ PCI_VDEVICE(INTEL, 0x1d41), LPC_PBG},
	{ PCI_VDEVICE(INTEL, 0x2310), LPC_DH89XXCC},
	{ PCI_VDEVICE(INTEL, 0x1e40), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e41), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e42), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e43), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e44), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e45), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e46), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e47), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e48), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e49), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e4a), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e4b), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e4c), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e4d), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e4e), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e4f), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e50), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e51), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e52), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e53), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e54), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e55), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e56), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e57), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e58), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e59), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e5a), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e5b), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e5c), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e5d), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e5e), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x1e5f), LPC_PPT},
	{ PCI_VDEVICE(INTEL, 0x8c40), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c41), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c42), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c43), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c44), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c45), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c46), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c47), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c48), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c49), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c4a), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c4b), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c4c), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c4d), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c4e), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c4f), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c50), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c51), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c52), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c53), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c54), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c55), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c56), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c57), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c58), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c59), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c5a), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c5b), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c5c), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c5d), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c5e), LPC_LPT},
	{ PCI_VDEVICE(INTEL, 0x8c5f), LPC_LPT},
#ifdef CONFIG_SYNO_AVOTON
	{ PCI_VDEVICE(INTEL, 0x1f38), LPC_AVN},
	{ PCI_VDEVICE(INTEL, 0x1f39), LPC_AVN},
	{ PCI_VDEVICE(INTEL, 0x1f3a), LPC_AVN},
	{ PCI_VDEVICE(INTEL, 0x1f3b), LPC_AVN},
#endif
	{ 0, },			/* End of list */
};
MODULE_DEVICE_TABLE(pci, lpc_ich_ids);

static void lpc_ich_restore_config_space(struct pci_dev *dev)
{
	if (lpc_ich_acpi_save >= 0) {
		pci_write_config_byte(dev, ACPICTRL, lpc_ich_acpi_save);
		lpc_ich_acpi_save = -1;
	}

	if (lpc_ich_gpio_save >= 0) {
		pci_write_config_byte(dev, GPIOCTRL, lpc_ich_gpio_save);
		lpc_ich_gpio_save = -1;
	}
}

static void __devinit lpc_ich_enable_acpi_space(struct pci_dev *dev)
{
	u8 reg_save;

	pci_read_config_byte(dev, ACPICTRL, &reg_save);
	pci_write_config_byte(dev, ACPICTRL, reg_save | 0x10);
	lpc_ich_acpi_save = reg_save;
}

static void __devinit lpc_ich_enable_gpio_space(struct pci_dev *dev)
{
	u8 reg_save;

	pci_read_config_byte(dev, GPIOCTRL, &reg_save);
	pci_write_config_byte(dev, GPIOCTRL, reg_save | 0x10);
	lpc_ich_gpio_save = reg_save;
}

static void __devinit lpc_ich_finalize_cell(struct mfd_cell *cell,
					const struct pci_device_id *id)
{
	cell->platform_data = &lpc_chipset_info[id->driver_data];
	cell->pdata_size = sizeof(struct lpc_ich_info);
}

#ifdef MY_DEF_HERE
/*
 * Now only Avoton use this way to control gpio
 * the others x64 platform syno_pch_lpc_gpio_pin is in
 * driver/pci/quirks.c
 * One day other x64 platforms can use this path
 */
static u32 gpiobase = 0;
static u32 *writable_pin = NULL;
static u32 SynoGpioCount = 0;

#if defined(CONFIG_SYNO_CEDARVIEW)
static u32 ich9_writable_pin[] = {1, 6, 7, 10, 15, 16, 17, 18, 20, 21, 24, 25, 29, 30, 31, 32, 33, 34, 35, 36, 37, 42, 43, 45, 46, 47, 49, 55, 57};
#else
static u32 ich9_writable_pin[] = {1, 6, 7, 10, 15, 16, 17, 18, 20, 21, 24, 25, 30, 31, 32, 33, 34, 35, 36, 37, 46, 47, 49, 55, 57};
#endif
static u32 c206_writable_pin[] = {0, 5, 16, 20, 21, 22, 34, 38, 48, 52, 54, 69, 70, 71};
static u32 c226_writable_pin[] = {5, 16, 18, 19, 20, 21, 23, 32, 33, 34, 35, 36, 37, 45};
static u32 avoton_writable_pin[] = {4, 5, 10, 27, 36, 49, 50, 51, 52, 53, 54, 58, 59};

#if defined(CONFIG_SYNO_AVOTON)
static u32 *int_disk_act_pin = NULL;
/* for avoton gpio we define that
 * CORE WELL gpio (GPIOS_X) start from 0 to 31
 * SUS WELL gpio (GPIO_SUSX) start from 32 to 63
 */
static u32 coreWellGpio = 0;
static u32 susWellGpio = 0;
static u32 avoton_disk_act_value[4] = {0};
static u32 avoton_disk_act_pin[4] = {52, 53, 54, 58};
static u32 avoton_corewell_gpioin_pin = 0x1800; //pin 11 12 for fan tach
static u32 avoton_suswell_gpioin_pin = 0x0;
#endif

#if defined(CONFIG_SYNO_AVOTON)
// avoton gpio pin 0~63
#define GPIO_MAX_PIN 63
#else
// other x64 platform gpio pin 0~95
#define GPIO_MAX_PIN 95
#endif

u32 syno_pch_lpc_gpio_pin(int pin, int *pValue, int isWrite)
{
	static DEFINE_SPINLOCK(lock);
	static unsigned long flags;
	int ret = -1;
    int i = 0;
    u32 addr_use_select, addr_io_select, addr_lvl;
	u32 val_lvl;
#if defined(CONFIG_SYNO_AVOTON)
	u32 *pVal_lvl = NULL;
	u32 gpioin_pin = 0;
#else
    u32 val_use_select, val_io_select;
#endif
    u32 mppPin = pin;
    u32 tmpVal;

    if (0 == gpiobase || ( pin < 0 || pin > GPIO_MAX_PIN )
			|| NULL == pValue) {
        printk("parameter error. gpiobase=%08X, pin=%d, pValue=%p\n", gpiobase, pin, pValue);
        goto END;
    }
	spin_lock_irqsave(&lock, flags);

	if (1 == isWrite) {
		// SynoGpioCount should follow the assigned array size of writable_pin
		while (i < SynoGpioCount) {
            if (pin == writable_pin[i]) {
                break;
            }
			i++;
        }
        if (i == SynoGpioCount) {
            printk("pin %d is protected by driver.\n", pin);
            goto UNLOCK;
        }
    }

    if (mppPin < 32) {
        addr_use_select = gpiobase + 0x00;
        addr_io_select = gpiobase + 0x04;
#if defined(CONFIG_SYNO_AVOTON)
		addr_lvl = gpiobase + 0x08;
		pVal_lvl = &coreWellGpio;
		gpioin_pin = avoton_corewell_gpioin_pin;
#else
        addr_lvl = gpiobase + 0x0c;
#endif
    } else if (mppPin < 64) {
        addr_use_select = gpiobase + 0x30;
        addr_io_select = gpiobase + 0x34;
#if defined(CONFIG_SYNO_AVOTON)
		addr_lvl = gpiobase + 0x88;
		pVal_lvl = &susWellGpio;
		gpioin_pin = avoton_suswell_gpioin_pin;
#else
        addr_lvl = gpiobase + 0x38;
#endif
        mppPin %= 32;
    } else {
        addr_use_select = gpiobase + 0x40;
        addr_io_select = gpiobase + 0x44;
        addr_lvl = gpiobase + 0x48;
        mppPin %= 32;
	}
/*
 * If Avoton GPIO pin set ouput, we can't read the value from register
 * so we need to store the gpio values in kernel instead of register
 */
#if defined(CONFIG_SYNO_AVOTON)
	if (0 == isWrite) {
        //out put value
			val_lvl = inl(addr_lvl);
		val_lvl &= gpioin_pin;
		*pVal_lvl |= val_lvl;
			*pValue = (*pVal_lvl & (1 << mppPin)) >> mppPin;
	} else {
		if (1 == *pValue) {
			tmpVal = 1 << mppPin;
			*pVal_lvl |= tmpVal;
			outl(*pVal_lvl, addr_lvl);
		} else {
			tmpVal = ~(1 << mppPin);
			*pVal_lvl &= tmpVal;
			outl(*pVal_lvl, addr_lvl);
		}
	}
#else
    if (0 == isWrite) {
        //change use select to GPIO
        val_use_select = inl(addr_use_select);
        tmpVal = 1 << mppPin;
        val_use_select |= tmpVal;
        outl(val_use_select, addr_use_select);

        //out put value
        val_lvl = inl(addr_lvl);

        *pValue = (val_lvl & (1 << mppPin)) >> mppPin;
    } else {
        //change use select to GPIO
        val_use_select = inl(addr_use_select);
        tmpVal = 1 << mppPin;
        val_use_select |= tmpVal;
        outl(val_use_select, addr_use_select);

        //change I/O select to output
        val_io_select = inl(addr_io_select);
        tmpVal = ~(1 << mppPin);
        val_io_select &= tmpVal;
        outl(val_io_select, addr_io_select);

        //out put value
        val_lvl = inl(addr_lvl);
        if (1 == *pValue) {
            tmpVal = 1 << mppPin;
            val_lvl |= tmpVal;
            outl(val_lvl, addr_lvl);
        } else {
            tmpVal = ~(1 << mppPin);
            val_lvl &= tmpVal;
            outl(val_lvl, addr_lvl);
        }
    }
#endif
    ret = 0;

UNLOCK:
	spin_unlock_irqrestore(&lock, flags);
END:
    return ret;
}
EXPORT_SYMBOL(syno_pch_lpc_gpio_pin);
#endif

#if defined(CONFIG_SYNO_AVOTON)
#define GPIO_UNDEF	0xFF
int SYNO_CTRL_HDD_ACT_NOTIFY(int index)
{
	int ret = 0;
	u32 pin = GPIO_UNDEF;
	int value = 0;

	switch (index) {
		case 0 ... 3:
			pin = int_disk_act_pin[index];
			avoton_disk_act_value[index] = !avoton_disk_act_value[index];
			value = avoton_disk_act_value[index];
			break;
		default:
			ret = -1;
			printk("%s: unsupported disk index [%d]\n", __FUNCTION__, index);
			goto END;
	}

	WARN_ON(GPIO_UNDEF == pin);
	syno_pch_lpc_gpio_pin(pin, &value, 1);
END:
	return ret;
}
EXPORT_SYMBOL(SYNO_CTRL_HDD_ACT_NOTIFY);
#endif

#ifdef MY_DEF_HERE
static int syno_gpio_init(struct pci_dev *dev)
{
	if (PCI_DEVICE_ID_INTEL_COUGARPOINT_LPC_C206 == dev->device) {
		writable_pin = c206_writable_pin;
		SynoGpioCount = ARRAY_SIZE(c206_writable_pin);
	} else if (PCI_DEVICE_ID_INTEL_LYNXPOINT_LPC_C226 == dev->device) {
		writable_pin = c226_writable_pin;
		SynoGpioCount = ARRAY_SIZE(c226_writable_pin);
	} else if (PCI_DEVICE_ID_INTEL_AVOTON_LPC == dev->device) {
		writable_pin = avoton_writable_pin;
		SynoGpioCount = ARRAY_SIZE(avoton_writable_pin);
#if defined(CONFIG_SYNO_AVOTON)
		int_disk_act_pin = avoton_disk_act_pin;
#endif
	} else {
		writable_pin = ich9_writable_pin;
		SynoGpioCount = ARRAY_SIZE(ich9_writable_pin);
	}

	return 0;
}
#endif

static int __devinit lpc_ich_init_gpio(struct pci_dev *dev,
				const struct pci_device_id *id)
{
	u32 base_addr_cfg;
	u32 base_addr;
	int ret;
	bool acpi_conflict = false;
	struct resource *res;

	/* Setup power management base register */
	pci_read_config_dword(dev, ACPIBASE, &base_addr_cfg);
	base_addr = base_addr_cfg & 0x0000ff80;
	if (!base_addr) {
		dev_err(&dev->dev, "I/O space for ACPI uninitialized\n");
		lpc_ich_cells[LPC_GPIO].num_resources--;
		goto gpe0_done;
	}

	res = &gpio_ich_res[ICH_RES_GPE0];
	res->start = base_addr + ACPIBASE_GPE_OFF;
	res->end = base_addr + ACPIBASE_GPE_END;
	ret = acpi_check_resource_conflict(res);
	if (ret) {
		/*
		 * This isn't fatal for the GPIO, but we have to make sure that
		 * the platform_device subsystem doesn't see this resource
		 * or it will register an invalid region.
		 */
		lpc_ich_cells[LPC_GPIO].num_resources--;
		acpi_conflict = true;
	} else {
		lpc_ich_enable_acpi_space(dev);
	}

gpe0_done:
	/* Setup GPIO base register */
	pci_read_config_dword(dev, GPIOBASE, &base_addr_cfg);
	base_addr = base_addr_cfg & 0x0000ff80;
	if (!base_addr) {
		dev_err(&dev->dev, "I/O space for GPIO uninitialized\n");
		ret = -ENODEV;
		goto gpio_done;
	}
#ifdef MY_DEF_HERE
	gpiobase = base_addr;
	syno_gpio_init(dev);
#endif

	/* Older devices provide fewer GPIO and have a smaller resource size. */
	res = &gpio_ich_res[ICH_RES_GPIO];
	res->start = base_addr;
	switch (lpc_chipset_info[id->driver_data].gpio_version) {
	case ICH_V5_GPIO:
	case ICH_V10CORP_GPIO:
		res->end = res->start + 128 - 1;
		break;
	default:
		res->end = res->start + 64 - 1;
		break;
	}

	ret = acpi_check_resource_conflict(res);
	if (ret) {
		/* this isn't necessarily fatal for the GPIO */
		acpi_conflict = true;
		goto gpio_done;
	}
	lpc_ich_enable_gpio_space(dev);

	lpc_ich_finalize_cell(&lpc_ich_cells[LPC_GPIO], id);
	ret = mfd_add_devices(&dev->dev, -1, &lpc_ich_cells[LPC_GPIO],
				1, NULL, 0);

gpio_done:
	if (acpi_conflict)
		pr_warn("Resource conflict(s) found affecting %s\n",
				lpc_ich_cells[LPC_GPIO].name);
	return ret;
}

static int __devinit lpc_ich_probe(struct pci_dev *dev,
				const struct pci_device_id *id)
{
	int ret;
	bool cell_added = false;

	ret = lpc_ich_init_gpio(dev, id);
	if (!ret)
		cell_added = true;

	/*
	 * We only care if at least one or none of the cells registered
	 * successfully.
	 */
	if (!cell_added) {
		lpc_ich_restore_config_space(dev);
		return -ENODEV;
	}

	return 0;
}

static void __devexit lpc_ich_remove(struct pci_dev *dev)
{
	mfd_remove_devices(&dev->dev);
	lpc_ich_restore_config_space(dev);
}

static struct pci_driver lpc_ich_driver = {
	.name		= "lpc_ich",
	.id_table	= lpc_ich_ids,
	.probe		= lpc_ich_probe,
	.remove		= __devexit_p(lpc_ich_remove),
};

static int __init lpc_ich_init(void)
{
	return pci_register_driver(&lpc_ich_driver);
}

static void __exit lpc_ich_exit(void)
{
	pci_unregister_driver(&lpc_ich_driver);
}

module_init(lpc_ich_init);
module_exit(lpc_ich_exit);

MODULE_AUTHOR("Aaron Sierra <asierra@xes-inc.com>");
MODULE_DESCRIPTION("LPC interface for Intel ICH");
MODULE_LICENSE("GPL");
