/******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

*******************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bitops.h>

#include "xcat98dx-switch.h"
#include "xcat98dx-eprom.h"
#include "xcat98dx-regs.h"
#include "xcat98dx-core.h"

void mvPpAddrComp(MV_U32 baseAddr, MV_U32 reg, MV_U32 *addr)
{
#define DEFAULT_REGION	2
	MV_U32 addressCompletion;
	MV_U32 region, regionValue, regionMask;

	/* read the contents of the addr completion register */
	addressCompletion =
		(MV_U32)MV_32BIT_LE_FAST(*((volatile MV_U32 *)baseAddr));

	/* check if the region need to be changed */
	/* calculate the region (bits 24,25) */
	region = (reg & 0x3000000) >> 24;

	/* the region val is the 8 MSB of the register addr */
	regionValue = (reg & 0xFF000000) >> 24;

	/* region 0 val is always 0 therefore we need to
	   check that the regionValue is also 0 */
	if (region == 0 && regionValue !=0 )
	{
		/* change to a region that can be changed */
		region = DEFAULT_REGION;
	}

	/* the region mask for read modify write */
	regionMask = ~(0xFF << (region * 8));

	/* check if update of region is neccessary */
	if (regionValue != (addressCompletion & regionMask) >> (region * 8))
	{
		/* unset all the specific region bits */
		addressCompletion &= regionMask;
		/* write the new region val to the right place */
		addressCompletion |= regionValue << (region * 8);

		/* write back the updated addr completion register */
		*((volatile MV_U32 *)baseAddr) =
			(MV_U32)MV_32BIT_LE_FAST(addressCompletion);
	}

	/* Calculate the PCI addr to return */
	/* Remove 8 MSB and add region ID + base addr */
	*addr = baseAddr | (region << 24) | (reg & 0x00FFFFFF);
}

void mvPpWriteReg(MV_U8 dev, MV_U32 reg, MV_U32 val)
{
	MV_U32 addr;
	mvPpAddrComp(KIRKWOOD_PP_VIRT_BASE, reg, &addr);
	*((volatile MV_U32 *)addr) = (MV_U32)MV_32BIT_LE_FAST(val);
}

MV_U32 mvPpReadReg(MV_U8 dev, MV_U32 reg)
{
	MV_U32 addr;
	MV_U32 val;

	mvPpAddrComp(MV_PP_BASE, reg, &addr);
	val = (MV_U32)(*((volatile MV_U32 *)addr));
	val = MV_32BIT_LE_FAST(val);

	return val;
}

void mvPpReadModWriteReg(MV_U8 dev, MV_U32 reg, MV_U32 mask, MV_U32 newVal)
{
	MV_U32 val = mvPpReadReg(dev, reg);
	val = (val & ~mask) | (newVal & mask);
	mvPpWriteReg(dev, reg, val);
}

void mvPpBitSet(MV_U8 dev, MV_U32 reg, MV_U32 mask)
{
	mvPpReadModWriteReg(dev, reg, mask, mask);
}

void mvPpBitReset(MV_U8 dev, MV_U32 reg, MV_U32 mask)
{
	mvPpReadModWriteReg(dev, reg, mask, 0);
}

/*
 * For xCat2:
 * Rev_id = 1 hard coded
 * Dev_id:
 *     dev_id[15:10]    = 6'b111001 (6h39)
 *     dev_id[9 :4]     = device_mode[5:0] (sample at reset)
 *     dev_id[3]        = device_mode[6] (sample at reset)
 *     dev_id[2]        = device_mode[7] (sample at reset)
 *     dev_id[1]        = bga_pkg  - package
 *     dev_id[0]        = ssmii_mode - package
 * As you can see, dev_id[15:10] is not board dependent
 * and distinguishes xcat from xcat2 (xcat has 0x37 on these bits).
 * dev_id[1:0] are also not board dependent and tells you
 * which package and FE/GE device is on board.
 * Of course, for CPSS you must follow the device matrix excel
 * which maps all the dev_id bits.
 *
 * Note: this function may be called on the very early boot stage, ==>
 *       printk() should not be used.
 */
MV_BOOL mvPpChipIsXCat2(void)
{
	MV_U32 devId, chipType;
	MV_BOOL isXCat2;

	devId = mvPpReadReg(MV_PP_DEV0, PRESTERA_DEV_ID_REG);
	chipType = (devId & MV_PP_CHIP_TYPE_MASK) >> MV_PP_CHIP_TYPE_OFFSET;

	if (chipType == MV_PP_CHIP_TYPE_XCAT)
		isXCat2 = MV_FALSE;
	else if (chipType == MV_PP_CHIP_TYPE_XCAT2)
		isXCat2 = MV_TRUE;
	else /* This code should be unreacheable. */
		BUG();

	return isXCat2;
}

MV_BOOL mvPpChipIsXCat(void)
{
	return !mvPpChipIsXCat2();
}

MV_BOOL mvPpChipIsXCat2Simple(void)
{
	MV_U32 devId, chipType;
	MV_BOOL isXCat2;

	devId = *(MV_U32 *)(KIRKWOOD_PP_PHYS_BASE+0x4c);
	devId = MV_32BIT_LE_FAST(devId);
	chipType = (devId & MV_PP_CHIP_TYPE_MASK) >> MV_PP_CHIP_TYPE_OFFSET;

	if (chipType == MV_PP_CHIP_TYPE_XCAT)
		isXCat2 = MV_FALSE;
	else if (chipType == MV_PP_CHIP_TYPE_XCAT2)
		isXCat2 = MV_TRUE;
	else /* This code should be unreacheable. */
		BUG();

	return isXCat2;
}

MV_U8 mvPpGetDevicesNumXCat(void)
{
	/* Currently only 1 dev is supported */
	return 1;
}

MV_U8 mvPpGetDevicesNum(void)
{
	if (mvPpChipIsXCat2Simple() == MV_TRUE)
		BUG(); /* xcat2 isn't supported */
	else
		return mvPpGetDevicesNumXCat();
}

/*
 * Config VLAN on set of ports
 */
static void mvSwitchVidxCfg(void)
{
	/* All ports are members of VIDX 1 except [dev 0, port 26] */
	mvPpWriteReg(MV_PP_DEV0, 0x0A100010, 0x1FFFFFFF);
	mvPpBitReset(MV_PP_DEV0, 0x0A100010, BIT(27) /* port 26 */);

	if (mvPpGetDevicesNum() > 1) {
		/* All ports are members of VIDX 1 except [dev 0, port 25] */
		mvPpWriteReg(MV_PP_DEV1, 0x0A100010, 0x1FFFFFFF);
		mvPpBitReset(MV_PP_DEV1, 0x0A100010, BIT(26) /* port 25 */);
	}
}

MV_STATUS mvPpReadVlanEntry(
	int dev,
	int entryNum,
	STRUCT_VLAN_ENTRY *vlanTableEntry)
{
	MV_U32 val, i;

#if defined PRESTERA_VLAN_INDIRECT
	if (setVLANTableCtrlReg(dev, entryNum, 0 /*Read*/) != MV_OK) {
		printk("%s: setVLANTableCtrlReg failed.\n", __func__);
		return MV_FAIL;
	}
#endif
	for (i = 0; i < 4; i++) {
		val = mvPpReadReg(dev, VLT_DATA_REG_OFFSET +
				entryNum * VLAN_ENTRY_SIZE + i * 4);
		/* Word 0 is in reg 3 and  */
		vlanTableEntry->VLTData[3-i] = val;
	}

	return MV_OK;
}

MV_STATUS mvPpSetVlanEntry(
	int dev,
	int entryNum,
	STRUCT_VLAN_ENTRY *vlanTableEntry)
{
	MV_U32 i=0;

	for (i = 0; i < 4; i++) {
		/* Word 0 is in reg 3 and  Word 3 is in reg 0, etc*/
		mvPpWriteReg(dev, VLT_DATA_REG_OFFSET +
			     entryNum * VLAN_ENTRY_SIZE + i * 4,
			     vlanTableEntry->VLTData[3-i]);
		return MV_FAIL;
	}

#if defined PRESTERA_VLAN_INDIRECT
	if (setVLANTableCtrlReg(dev, entryNum, 1 /*Write*/) != MV_OK)
		return MV_FAIL;
#endif
	return MV_OK;
}

MV_STATUS setCpuAsVLANMember(int dev, int vlan)
{
	STRUCT_VLAN_ENTRY vlanTableEntry;

	if (mvPpReadVlanEntry(dev, vlan, &vlanTableEntry) != MV_OK)
		return MV_FAIL;

	vlanTableEntry.VLTData[3] |= (1 << CPU_VLAN_MEMBER_BIT) |
				     (1 << IPv4_CTRL_TO_CPU_ENABLE) |
				     (0x2 << 6 /* UnregisteredIPv4MulticastCmd */);

	return mvPpSetVlanEntry(dev, vlan, &vlanTableEntry);
}

/*
 * Config basic switch CPU port parameters.
 */
void mvPpCpuPortConfig(MV_U8 dev)
{
	/* Mg Global Control: set SelPortSDMA = 0 and PowerSave = 0 */
	mvPpReadModWriteReg(dev, PRESTERA_GLOBAL_CTRL_REG, BIT(19) | BIT(20), 0);

	/* Set ref_clk_125_sel to PLL */
	mvPpReadModWriteReg(dev, PRESTERA_EXT_GLOBAL_CFG_REG, BIT(10), BIT(10));

	/* Set CPUPortActive = 1, CPUPortIFType = 2, MIBCountMode = 1. */
	mvPpWriteReg(dev, PRESTERA_CPU_PORT_GLOBAL_CFG_REG, 0xd);

	/* Set R0_Active = 0, RGPP_TEST = 0, GPP_Active = 1 */
	mvPpReadModWriteReg(dev, PRESTERA_DEV_CONFIG_REG,
			(BIT(18) | BIT(19) | BIT(20)), BIT(18));

	mvPpReadModWriteReg(dev, PRESTERA_SAMPLE_AT_RESET_REG,
			(BIT(10) | BIT(11) | BIT(12)), (BIT(10) | BIT(11)));

	/*
	 * Set PortMacControl fot port 63:
	 * PcsEn = 0, UseIntClkforEn = 1, PortMACReset = 0,
	 * CollisionOnBackPressureCntEn=1
	 */
	mvPpWriteReg(dev, PRESTERA_CPU_PORT_MAC_CTRL_REG(3), 0x300);
	mvPpWriteReg(dev, PRESTERA_CPU_PORT_MAC_CTRL_REG(2), 0x4010);
	mvPpWriteReg(dev, PRESTERA_CPU_PORT_MAC_CTRL_REG(1), 0x1f87);
	mvPpWriteReg(dev, PRESTERA_CPU_PORT_MAC_CTRL_REG(0), 0x8be5);
}

/*
 * Configures PP CPU_Port to be DSA tagged (Cascade port).
 */
void mvPpCpuPortCascadeModeSet(MV_U32 dev)
{
	mvPpReadModWriteReg(dev, CASCADE_AND_HEADER_CONFIG_REG, BIT(31), BIT(31));
}

/*
 * Disable two-byte prepending header to every incoming packet, because
 * this two-bytes are prepended by KW-GbE ethernet port unconditionally.
 */
void mvPpCpuPortTwoBytePrependDisable(void)
{
	mvPpReadModWriteReg(0, CASCADE_AND_HEADER_CONFIG_REG, BIT(28), 0);
}

MV_U32 mvPpMruGet(MV_U32 dev, MV_U32 port)
{
	MV_U32 addr, val;

	if (port < 28)
		addr = PP_PORT_MAC_CTRL_REG0(port);
	else if (port == PP_CPU_PORT_NUM)
		addr = PP_CPU_PORT_MAC_CTRL_REG;
	else {
		printk("%s: Wrong port (%d).\n", __func__, port);
		return -1;
	}

	val = mvPpReadReg(dev, addr);
	val &= (0x1FFF << 2); /* MRU in double-bytes */
	val /= 2; /* MRU in bytes */
	return val;
}

MV_STATUS mvPpMruSet(MV_U32 dev, MV_U32 port, MV_U32 mruBytes)
{
	/* FrameSizeLimit mask */
	MV_U32 mruBits = ((mruBytes / 2) << 2) & (0x1FFF << 2);
	MV_U32 val, addr;

	if (port < 28)
		addr = PP_PORT_MAC_CTRL_REG0(port);
	else if (port == PP_CPU_PORT_NUM)
		addr = PP_CPU_PORT_MAC_CTRL_REG;
	else {
		printk("%s: Wrong port (%d).\n", __func__, port);
		return MV_FAIL;
	}

	/*
	 * Configure MRU for Prestera Switch CPU_Port (only for dev 0)
	 */
	val = mvPpReadReg(dev, addr);
	val &= ~(0x1FFF << 2); /* clean size limit bits */
	val |= mruBits;
	mvPpWriteReg(dev, addr, val);

	return MV_OK;
}

MV_STATUS mvPpMruSetAllPorts(MV_U32 mruBytes)
{
	MV_U32 dev, port, numOfDevs;

	numOfDevs = mvPpGetDevicesNum();

	/* mru  = mru + 8; */ /* due to DSA tag */

	for (dev = 0; dev < numOfDevs; dev++)
		for (port = 0; port < 28; port++)
			if (mvPpMruSet(dev, port, mruBytes) != MV_OK) {
				printk("%s: mvPpMruSet(mruBytes=%d) failed.\n",
						__func__, mruBytes);
				return MV_FAIL;
			}

	if (mvPpMruSet(MV_PP_DEV0, PP_CPU_PORT_NUM, mruBytes) != MV_OK) {
		printk ("%s: mvPpMruSet(%d) failed.\n", __func__, mruBytes);
		return MV_FAIL;
	}

	return MV_OK;
}

void mvPpMruPrintAllPorts(void)
{
	MV_U32 dev, port, numOfDevs, mruBytes;

	numOfDevs = mvPpGetDevicesNum();
	for (dev = 0; dev < numOfDevs; dev++) {
		for (port = 0; port < 28; port++) {
			mruBytes = mvPpMruGet(dev, port);
			printk("dev%d:port%d: MRU(bytes) = %d\n", dev, port, mruBytes);
		}
	}

	mruBytes = mvPpMruGet(MV_PP_DEV0, PP_CPU_PORT_NUM);
	printk("dev%d:port%d: MRU(bytes) = %d\n",
	       MV_PP_DEV0, PP_CPU_PORT_NUM, mruBytes);
}

MV_U32 mvPresteraMibCounterRead(int dev, int portNum, unsigned int mibOffset,
				MV_U32* pHigh32)
{
	MV_U32 valLow32, valHigh32;

	valLow32 = mvPpReadReg(dev, PRESTERA_MIB_REG_BASE(portNum) + mibOffset);

	/* Implement FEr ETH. Erroneous Value when Reading the Upper 32-bits    */
	/* of a 64-bit MIB Counter.                                             */
	if (mibOffset == PRESTERA_MIB_GOOD_OCTETS_RECEIVED_LOW ||
	    mibOffset == PRESTERA_MIB_GOOD_OCTETS_SENT_LOW) {
		valHigh32 =	mvPpReadReg(dev,
				PRESTERA_MIB_REG_BASE(portNum) + mibOffset);
		if (pHigh32 != NULL)
			*pHigh32 = valHigh32;
	}

	return valLow32;
}

void mvPpReadPortMibCntCpuPort(MV_U32 dev)
{
	MV_U32 regVal, tmp;

	/*
	 * Check if CPU_Port is active.
	 */
	regVal = mvPpReadReg(dev, 0xA0);
	tmp = regVal & 1;
	if (tmp == 0) {
		printk("CPU_Port is inactive (CPUPortActive = 0).\n");
		return;
	}

	printk("CPU Port GoodFramesSent           = %d\n",
			mvPpReadReg(dev, 0x60));
	printk("CPU Port MACTransErrorFramesSent  = %d\n",
			mvPpReadReg(dev, 0x64));
	printk("CPU Port GoodOctetsSent           = %d\n",
			mvPpReadReg(dev, 0x68));
	printk("CPU Port Rx Internal Drop         = %d\n",
			mvPpReadReg(dev, 0x6C));
	printk("CPU Port GoodFramesReceived       = %d\n",
			mvPpReadReg(dev, 0x70));
	printk("CPU Port BadFramesReceived        = %d\n",
			mvPpReadReg(dev, 0x74));
	printk("CPU Port GoodOctetsReceived       = %d\n",
			mvPpReadReg(dev, 0x78));
	printk("CPU Port BadOctetsReceived        = %d\n",
			mvPpReadReg(dev, 0x7C));
	printk("CPU Port Global Configuration     = %d\n",
			mvPpReadReg(dev, 0xA0));
}

void mvPpReadStackingPortMibCounters(MV_U32 dev, MV_U32 port)
{
	MV_U32 mibBase = 0x09300000 + (port - 24) * 0x20000;
	MV_U32 val, high;

	if (port < 24 || port > 27) {
		printk("%s: Wrong port number (%d).\n", __func__, port);
		return;
	}

	printk("Reading Stacking Port counters (Port %d Device %d)\n\n", port, dev);
	printk("Port MIB base address: 0x%08x\n", mibBase);

	val = mvPpReadReg(dev, mibBase);
	high = mvPpReadReg(dev, mibBase + 0x4);
	printk("GoodOctetsReceived          = 0x%08x%08x\n", high, val);

	val = mvPpReadReg(dev, mibBase + 0x8);
	printk("BadOctetsReceived           = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0xC);
	printk("CRCErrorsSent               = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x10);
	printk("GoodUnicastFramesReceived   = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x14);
	printk("Reserved                    = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x18);
	printk("BroadcastFramesReceived     = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x1C);
	printk("MulticastFramesReceived     = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x20);
	printk("Frames64 Octets             = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x24);
	printk("Frames65to127 Octets        = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x28);
	printk("Frames128to255 Octets       = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x2C);
	printk("Frames256to511 Octets       = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x30);
	printk("Frames512to1023 Octets      = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x34);
	printk("Frames1024toMax Octets      = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x38);
	high = mvPpReadReg(dev, mibBase + 0x3C);
	printk("GoodOctetsSent              = 0x%08x%08x\n", high, val);

	val = mvPpReadReg(dev, mibBase + 0x40);
	printk("UnicastFramesSent           = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x44);
	printk("Reserved                    = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x48);
	printk("MulticastFramesSent         = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x4C);
	printk("BroadcastFramesSent         = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x50);
	printk("Reserved                    = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x54);
	printk("FlowControlSent             = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x58);
	printk("FlowControlReceived         = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x5C);
	printk("ReceivedFIFOOverrun         = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x60);
	printk("Undersize                   = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x64);
	printk("Fragments                   = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x68);
	printk("Oversize                    = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x6C);
	printk("Jabber                      = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x70);
	printk("RxErrorFrameReceived        = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x74);
	printk("BadCRC                      = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x78);
	printk("Reserved                    = %u\n", val);

	val = mvPpReadReg(dev, mibBase + 0x7C);
	printk("Reserved                    = %u\n", val);
}

void mvPresteraReadPortMibCounters(int port)
{
	MV_U32  regValue, regValHigh;
	MV_U8   dev = port / 28;
	MV_U32  portNum = port % 28;

	if (port == PP_CPU_PORT_NUM) {
		printk("CPU Port is a special port:\n");
		mvPpReadPortMibCntCpuPort(MV_PP_DEV0);
		return;
	}

	if (portNum == 24 || portNum == 25 || portNum == 26 || portNum == 27) {
		mvPpReadStackingPortMibCounters(dev, portNum);
		return;
	}

	pr_info("\n\t Prestera Port #%d MIB Counters (Port %d Device %d)\n\n",
		port, portNum, dev);
	printk("Port MIB base address: 0x%08x\n",PRESTERA_MIB_REG_BASE(portNum));

	printk("GoodFramesReceived          = %u\n",
			mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_GOOD_FRAMES_RECEIVED, NULL));
	printk("BroadcastFramesReceived     = %u\n",
			mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_BROADCAST_FRAMES_RECEIVED, NULL));
	printk("MulticastFramesReceived     = %u\n",
			mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_MULTICAST_FRAMES_RECEIVED, NULL));

	regValue = mvPresteraMibCounterRead(dev, portNum,
				PRESTERA_MIB_GOOD_OCTETS_RECEIVED_LOW,
			&regValHigh);
	printk("GoodOctetsReceived          = 0x%08x%08x\n",
			regValHigh, regValue);

	printk("\n");
	printk("GoodFramesSent              = %u\n",
			mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_GOOD_FRAMES_SENT, NULL));
	printk("BroadcastFramesSent         = %u\n",
			mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_BROADCAST_FRAMES_SENT, NULL));
	printk("MulticastFramesSent         = %u\n",
			mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_MULTICAST_FRAMES_SENT, NULL));

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_GOOD_OCTETS_SENT_LOW,
			&regValHigh);
	printk("GoodOctetsSent              = 0x%08x%08x\n", regValHigh, regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_SENT_MULTIPLE, NULL);
	printk("SentMultiple                = %u\n", regValue);

	printk("SentDeferred                = %u\n",
			mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_SENT_DEFERRED, NULL));

	printk("\n\t FC Control Counters\n");

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_GOOD_FC_RECEIVED, NULL);
	printk("GoodFCFramesReceived        = %u\n", regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_RECEIVED_FIFO_OVERRUN, NULL);
	printk("ReceivedFifoOverrun         = %u\n", regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_FC_SENT, NULL);
	printk("FCFramesSent                = %u\n", regValue);

	printk("\n\t RX Errors\n");

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_BAD_OCTETS_RECEIVED, NULL);
	printk("BadOctetsReceived           = %u\n", regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_UNDERSIZE_RECEIVED, NULL);
	printk("UndersizeFramesReceived     = %u\n", regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_FRAGMENTS_RECEIVED, NULL);
	printk("FragmentsReceived           = %u\n", regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_OVERSIZE_RECEIVED, NULL);
	printk("OversizeFramesReceived      = %u\n", regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_JABBER_RECEIVED, NULL);
	printk("JabbersReceived             = %u\n", regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_RX_ERROR_FRAME_RECEIVED, NULL);
	printk("RxErrorFrameReceived        = %u\n", regValue);

	/*regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_BAD_CRC_EVENT, NULL);
	  printk("BadCrcReceived              = %u\n", regValue);*/

	printk("\n\t TX Errors\n");

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_TX_FIFO_UNDERRUN_AND_CRC, NULL);
	printk("TxFifoUnderrunAndCRC        = %u\n", regValue);

	regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_EXCESSIVE_COLLISION, NULL);
	printk("TxExcessiveCollisions       = %u\n", regValue);

	/*regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_COLLISION, NULL);
	  printk("TxCollisions                = %u\n", regValue);

	  regValue = mvPresteraMibCounterRead(dev, portNum,
			PRESTERA_MIB_LATE_COLLISION, NULL);
	  printk("TxLateCollisions            = %u\n", regValue);*/

	printk("\n");
	/*regValue = MV_REG_READ( PRESTERA_RX_DISCARD_PKTS_CNTR_REG(port));
	  printk("Rx Discard packets counter    = %u\n", regValue);

	  regValue = MV_REG_READ(PRESTERA_RX_OVERRUN_PKTS_CNTR_REG(port));
	  printk("Rx Overrun packets counter  = %u\n", regValue);*/
}

MV_U32 mvPpGetDevId(void)
{
	MV_U32 devIdReg = 0;

	/* Read deviceId register */
	devIdReg  = *(MV_U32*)(MV_PP_BASE + 0x4C);
	devIdReg  = MV_32BIT_LE(devIdReg);

	return devIdReg;
}

MV_U32 mvPpGetXcatChipRev(void)
{
	MV_U32 devIdReg = 0;

	/* Read deviceId register */
	devIdReg  = *(MV_U32*)(MV_PP_BASE + 0x4C);
	devIdReg  = MV_32BIT_LE(devIdReg);
	devIdReg &= 0xF;

	return devIdReg;
}

MV_U32 mvPpGetXcat2ChipRev(void)
{
	/* Read DeviceId status register. */
	MV_U32 revId = mvPpReadReg(0, 0x8f8240);

	/* Read JTAG Revision status register. */
	revId >>= 20;
	revId &= 0xF;
	revId -= 1;

	return revId;
}

MV_U32 mvPpGetChipRev(void)
{
	MV_U32 rev;

	if (mvPpChipIsXCat2() == MV_TRUE)
		rev = mvPpGetXcat2ChipRev();
	else
		rev = mvPpGetXcatChipRev();

	return rev;
}

MV_BOOL mvPpIsChipFE(void)
{
	MV_U32 devIdReg = 0;

	/* Read deviceId register */
	devIdReg   = *(MV_U32*)(MV_PP_BASE + 0x4C);
	devIdReg   = MV_32BIT_LE(devIdReg);
	devIdReg  &= 0x10;
	devIdReg >>= 4;

	return devIdReg;
}

MV_BOOL mvPpIsChipGE(void)
{
	return !mvPpIsChipFE();
}

void mvPpConfigNetworkPorts(void)
{
#ifdef CONFIG_MV_XCAT98DX_WARM_RESET
	unsigned int data;
	/*
	 * Check if called after Warm reset, if not - up WarmReset Flag.
	 */
	data = mvPpReadReg(0, MV_WARM_RESET_REG);
	if (data != MV_WARM_RESET_MAGIC) {
#endif
		if (mvPpIsChipFE() == MV_TRUE)
			simulate_PP_EEPROM_2122_24FE();
		else
			simulate_PP_EEPROM_4122_24GE_PHY_A0();
#ifdef CONFIG_MV_XCAT98DX_WARM_RESET
	} else
		mvPpWriteReg(0, MV_WARM_RESET_REG, MV_WARM_RESET_MAGIC);
#endif
	mvSwitchVidxCfg();

	/*
	 * SelPortSDMA bit to CPU port
	 * PowerSave bit to Power_Saving_Disabled
	 */
	mvPpBitReset(MV_PP_DEV0, PRESTERA_GLOBAL_CTRL_REG, BIT(19) | BIT(20));

	/*
	 * Enable ARP to reach CPU
	 */
	mvPpBitSet(MV_PP_DEV0, 0x02040000, BIT(8));
}
