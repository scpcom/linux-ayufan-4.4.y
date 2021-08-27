/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
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
*******************************************************************************/

#include "xcat98dx-core.h"
#include "xcat98dx-regs.h"
#include "xcat98dx-eprom.h"
#include "mvOs.h"

#define DB(x)

/*******************************************************************************
 * MV_PP_STACK_BASE_ADDR
 */
static INLINE MV_U32 MV_PP_STACK_BASE_ADDR(MV_U32 stackPort)
{
    MV_U32 stackPortBase;

    if (stackPort < 24 || stackPort > 27)
    {
        mvOsPrintf("%s: Error: stackPort is %d.\n", __func__, stackPort);
        return  MV_FAIL;
    }

    stackPortBase = 0x0A806000 + (stackPort - 24) * 0x400;
    return stackPortBase;
}

/*******************************************************************************
 * MV_PP_STACK_REG
 */
static INLINE MV_U32 MV_PP_STACK_REG(MV_U32 stackPort, MV_U32 off)
{
    MV_U32 stackPortBase;
    MV_U32 addr;

    stackPortBase = MV_PP_STACK_BASE_ADDR(stackPort);
    addr = stackPortBase | off;
    return addr;
}

/*******************************************************************************
 * mvPpStackPortForceLinkUp
 */
static INLINE void mvPpStackPortForceLinkUp(MV_U32 dev, MV_U32 stackPort)
{
    DB(mvOsPrintf("%s: dev = %d, stackPort = %d.\n", __func__, dev, stackPort));
    mvPpBitReset(dev, MV_PP_STACK_REG(stackPort, 0x00C), BIT0);
}

/*******************************************************************************
 * mvPpStackPortForceLinkDown
 */
static INLINE void mvPpStackPortForceLinkDown(MV_U32 dev, MV_U32 stackPort)
{
    DB(mvOsPrintf("%s: dev = %d, stackPort = %d.\n", __func__, dev, stackPort));
    mvPpBitSet(dev, MV_PP_STACK_REG(stackPort, 0x00C), BIT0);
}

/*******************************************************************************
 * mvPpEeprom_DB_XCat2A1_QFP_24FE_4GP_do
 */
MV_STATUS mvPpEeprom_DB_XCat2A1_QFP_24FE_4GP_do(MV_U32 dev)
{
    /* CT 24FE_QFP_Confi */
    /* This file was generated using ConfiguratorVersion 0.1.5.1 0.1.5.1 */
    /* This file was generated using built in Device Database */
    /* File Generated on 3/1/2011 15:55:49 */
    /* 24 FE -- with 88E3083 */
    /* 4x SGMII2.5G -- No PHY */
    /* set SMI0 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004200, 0x70000);
    /* set SMI1 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004200, 0x70000);
    /* Pipe0 access DFX Setting- Pipe Select */
    mvOsDelay(1); mvPpWriteReg(dev, 0x308F8004, 0x1);
    /* PCL TCAM-DFX Setting */
    mvOsDelay(1); mvPpWriteReg(dev, 0x30804050, 0x20003);
    /* RTR TCAM-DFX Setting */
    mvOsDelay(1); mvPpWriteReg(dev, 0x3080C050, 0x20003);
    /* changed in xCat2, GPP PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8304, 0x48);
    /* changed in xCat2, SSMII PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8314, 0x48);
    /* changed in xCat2, SMI PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8318, 0x48);
    /* changed in xCat2, RGMII PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8310, 0x48);
    /* changed in xCat2, LED PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8300, 0x48);
    /* when TDM is NC: disconnect TDM SLIC PAD */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8320, 0x1FC);
    /* disable PECL receiver and common_0_PECL_EN=CMOS */
    mvOsDelay(1); mvPpWriteReg(dev, 0x9C, 0x61B0CC3);
    /* Power Down Unused SERDES */
    /* SD_Slumber=1 */

    mvOsDelay(1); mvPpBitSet(dev, 0x9800004, BIT6);
    mvOsDelay(1); mvPpBitSet(dev, 0x9800404, BIT6);
    mvOsDelay(1); mvPpBitSet(dev, 0x9801004, BIT6);
    mvOsDelay(1); mvPpBitSet(dev, 0x9801404, BIT6);
    mvOsDelay(1); mvPpBitSet(dev, 0x9802004, BIT6);
    mvOsDelay(1); mvPpBitSet(dev, 0x9802404, BIT6);

    /* Set PHY address registers */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004030, 0xA418820);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4804030, 0x16A4A0E6);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004030, 0x207B9AC);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5804030, 0xE629062);
    /*  Set SMI interface 0 to 16 ports and SMI 1 to 8 ports */
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x4004200, 0xC0000, 0x80000);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x5004200, 0xC0000, 0x0);
    /* Set Extended Control Register */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5C, 0x7405);
    /* Set Device Cfg */
    mvOsDelay(1); mvPpWriteReg(dev, 0x28, 0x7E000000);
    /* For QFP (GE and FE): disconnect RGMII PADs */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8310, 0x148);
    /* DP Clock Select */
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0xA806090, 0xC, 0x4);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0xA806490, 0xC, 0x4);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0xA806890, 0xC, 0x4);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0xA806C90, 0xC, 0x4);
    /* Deassert GIG MAC Reset */
    mvOsDelay(1); mvPpWriteReg(dev, 0xA806008, 0xC008);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA806408, 0xC008);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA806808, 0xC008);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA806C08, 0xC008);
    /* Disable AN */
    mvOsDelay(1); mvPpWriteReg(dev, 0xA80600C, 0x9068);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA80640C, 0x9068);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA80680C, 0x9068);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA806C0C, 0x9068);
    /* Disable Gig Pcs */
    mvOsDelay(1); mvPpWriteReg(dev, 0xA800008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA802008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA804008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA800408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA802408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA804408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA800808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA802808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA804808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA800C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA802C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA804C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA801008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA803008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA805008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA801408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA803408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA805408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA801808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA803808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA805808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA801C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA803C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA805C08, 0xC000);
    /* Restart Autoneg */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x13200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x23200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x33200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x43200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x53200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x63200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x73200);
    /* Restart Autoneg */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x83200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x93200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xA3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xB3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xC3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xD3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xE3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xF3200);
    /* Restart Autoneg */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x13200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x23200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x33200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x43200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x53200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x63200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x73200);
    /* Port[11..0], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004100, 0x2D75FF00);
    /* Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004100, 0x2D75FF00);
    /* Port[11..0], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004104, 0x44442222);
    /* Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004104, 0x44442222);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4804104, 0x33335555);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5804104, 0x33335555);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004108, 0xE000E000);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004108, 0xE000E000);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4804108, 0x0);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5804108, 0x0);
    /* Act_ & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x400410C, 0x6000);
    /* Act_ & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x500410C, 0x6000);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x480410C, 0x0);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x580410C, 0x0);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4804100, 0x11000);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5804100, 0x11000);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5005110, 0x10);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4005110, 0x10);
    /* Device Enable */
    mvOsDelay(1); mvPpBitSet(dev, PRESTERA_GLOBAL_CTRL_REG, BIT0);

    return MV_OK;
}

/*******************************************************************************
 * mvPpEeprom_DB_XCat2A1_QFP_24FE_4GP
 */
MV_STATUS mvPpEeprom_DB_XCat2A1_QFP_24FE_4GP(void)
{
    if (mvPpEeprom_DB_XCat2A1_QFP_24FE_4GP_do(MV_PP_DEV0) != MV_OK)
    {
        mvOsPrintf("%s: emulation failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpEeprom_DB_XCat2A1_48FE_4GP
 */
MV_STATUS mvPpEeprom_DB_XCat2A1_48FE_4GP(void)
{
    if (mvPpEeprom_DB_XCat2A1_48FE_4GP_dev(MV_PP_DEV0) != MV_OK)
    {
        mvOsPrintf("%s: mvPpEeprom_DB_XCat2A1_48FE_4GP_dev(%d) failed.\n.",
                   __func__, MV_PP_DEV0);
        return MV_FAIL;
    }

    if (mvPpEeprom_DB_XCat2A1_48FE_4GP_dev(MV_PP_DEV1) != MV_OK)
    {
        mvOsPrintf("%s: mvPpEeprom_DB_XCat2A1_48FE_4GP_dev(%d) failed.\n.",
                   __func__, MV_PP_DEV0);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * mvPpEeprom_DB_XCat2A1_48FE_4GP_dev
 */
MV_STATUS mvPpEeprom_DB_XCat2A1_48FE_4GP_dev(MV_U32 dev)
{
    MV_U32 serdesBase;
    MV_U32 stackPort0, stackPort1;

    if (dev == MV_PP_DEV0)
    {
        /* PP dev 0 uses stack ports 26-27. */
        serdesBase = 0x09804000;

        /* Stacking ports base address. */
        /* 0x0A806000 + (m-24)*0x400: where m (24-27) represents Port */
        stackPort0 = 26;
        stackPort1 = 27;
    }
    else
    {
        /* PP dev 1 uses stack ports 24-25. */
        serdesBase = 0x09803000;

        /* Stacking ports base address. */
        /* 0x0A806000 + (m-24)*0x400: where m (24-27) represents Port */
        stackPort0 = 24;
        stackPort1 = 25;
    }

    /* This file was generated using ConfiguratorVersion 0.1.5.0 0.1.5.0 */
    /* This file was generated using built in Device Database */
    /* File Generated on 5-12-2010 11:48:28 */
    /* 24 SSSMII -- with 88E3083 */
    /* 4x SGMII2.5G -- No PHY , P24,25 LR , P26,27 SR */

    /* set SMI0 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004200, 0x70000);
    /* set SMI1 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004200, 0x70000);
    /* Pipe0 access DFX Setting- Pipe Select  */
    mvOsDelay(1); mvPpWriteReg(dev, 0x308F8004, 0x1);
    /* PCL TCAM-DFX Setting */
    mvOsDelay(1); mvPpWriteReg(dev, 0x30804050, 0x20003);
    /* RTR TCAM-DFX Setting */
    mvOsDelay(1); mvPpWriteReg(dev, 0x3080C050, 0x20003);
    /* changed in xCat2, GPP PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8304, 0x48);
    /* changed in xCat2, SSMII PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8314, 0x48);
    /* changed in xCat2, SMI PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8318, 0x48);
    /* changed in xCat2, RGMII PAD control */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8310, 0x48);
    /* changed in xCat2, LED PAD control        */
    mvOsDelay(1); mvPpWriteReg(dev, 0x8F8300, 0x48);
    /* disable PECL receiver and common_0_PECL_EN=CMOS */
    mvOsDelay(1); mvPpWriteReg(dev, 0x9C, 0x61B0CC3);
    /* Power Down Unused SERDES */
    /* SD_Slumber=1 */
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x9800004, 0x40, 0x40);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x9800004, 0x40, 0x40);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x9800004, 0x40, 0x40);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x9800004, 0x40, 0x40);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x9800004, 0x40, 0x40);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x9800004, 0x40, 0x40);
    /* Set PHY address registers */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004030, 0xA418820);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4804030, 0x16A4A0E6);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004030, 0x207B9AC);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5804030, 0xE629062);
    /*  Set SMI interface 0 to 16 ports and SMI 1 to 8 ports */
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x4004200, 0xC0000, 0x80000);
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x5004200, 0xC0000, 0x0);
    /* Set Extended Control Register */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5C, 0x807405);
    /* Set Device Cfg */
    mvOsDelay(1); mvPpWriteReg(dev, 0x28, 0x7E000000);
    /* Power up 3.125G SERDESs */
    /* Set SERDES ref clock register */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x000, 0xBE80);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x400, 0xBE80);
    /* Wait 10mSec */
    mvOsDelay(10);
    /* Deactivate sd_reset */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x004, 0x8);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x404, 0x8);
    /* Wait for Calibration done (0x09800008 bit 3) */
    mvOsDelay(10);
    /* Reference Ferquency select = 62.5MHz   ;Use registers bits to control speed configuration                          */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x20C, 0x800A);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x60C, 0x800A);
    /* Transmitter/Reciver Divider force, interpulator force; 1.25G: intpi = 25uA , VCO divided by 4 ; 2.5G: intpi = 25uA , VCO divided by 2  ; 3.125G: intpi = 30uA , VCO divided by 2 ; 3.75G: intpi = 20uA , VCO not divided; 6.25G: intpi = 30uA , VCO not divided; 5.15G: intpi = 25uA , VCO not divided */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x210, 0x5515);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x610, 0x5515);
    /* Force FbDiv/RfDiv */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x214, 0xA164);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x614, 0xA164);
    /* Force: PLL Speed, sel_v2i, loadcap_pll,sel_fplres */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x218, 0xFBAA);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x618, 0xFBAA);
    /* icp force */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x21C, 0x8720);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x61C, 0x8720);
    /*  0 = kvco-2 */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x3CC, 0x2000);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x7CC, 0x2000);
    /* External TX/Rx Impedance changed from 6 to 0 while auto calibration results are used  - based on lab measurments it seems that we need to force the auto imedance calibration values */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x22C, 0x0);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x62C, 0x0);
    /* Auto KVCO,  PLL is not forced to max speed during power up sequence -  */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x230, 0x0);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x630, 0x0);
    /* Sampler OS Scale was changed from 5mV/Step to 3.3mV/Step; RX_IMP_VTHIMCAL was chnge from 3 to 0 */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x234, 0x4000);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x634, 0x4000);
    /* Use value wiritten to register for process calibration instead of th eauto calibration; Select process from register */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x23C, 0x18);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x63C, 0x18);
    /* DCC should be dissabled at baud 3.125 and below = 8060 */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x250, 0x80C0);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x650, 0x80C0);
    mvOsDelay(5);
    /* DCC should be dissabled at baud 3.125 and below = 8060 */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x250, 0x8060);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x650, 0x8060);
    /* PE Setting */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x254, 0x7200);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x654, 0x7200);
    /* PE Type */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x258, 0x0);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x658, 0x0);
    /* selmupi/mupf - low value for lower baud */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x27C, 0x905A);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x67C, 0x905A);
    /* DTL_FLOOP_EN = Dis */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x280, 0x800);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x680, 0x800);
    /* FFE Setting */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x28C, 0x236);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x68C, 0x236);
    /* Slicer Enable; Tx  Imp was changed from 50ohm to 43ohm */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x35C, 0x423F);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x75C, 0x423F);
    /* Not need to be configure - Same as default  */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x364, 0x5555);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x764, 0x5555);
    /* Disable ana_clk_det */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x36C, 0x0);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x76C, 0x0);
    /* Configure rx_imp_vthimpcal to 0x0 (default value = 0x3); Configure Sampler_os_scale to 3.3mV/step (default value = 5mV/step) */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x234, 0x4000);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x634, 0x4000);
    /* Configure IMP_VTHIMPCAL to 56.7ohm (default value = 53.3 ohm); Configure cal_os_ph_rd to 0x60 (default value = 0x0); Configure Cal_rxclkalign90_ext to use an external ovride value */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x228, 0xE0C0);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x628, 0xE0C0);
    /* Reset dtl_rx ; Enable ana_clk_det */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x36C, 0x8040);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x76C, 0x8040);
    /* Un reset dtl_rx */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x36C, 0x8000);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x76C, 0x8000);
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x224, 0x0);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x624, 0x0);
    /* CAL Start */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x224, 0x8000);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x624, 0x8000);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x224, 0x0);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x624, 0x0);
    /* Wait for RxClk_x2 */
    mvOsDelay(10);
    /* Set RxInit to 0x1 (remember that bit 3 is already set to 0x1) */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x004, 0x18);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x404, 0x18);
    /* Wait for p_clk = 1 and p_clk = 0 */
    mvOsDelay(10);
    /* Set RxInit to 0x0 */
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x004, 0x8);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x404, 0x8);
    /* Wait for ALL PHY_RDY = 1 (0x09800008 bit 0) */
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x004, 0x28);
    mvOsDelay(1); mvPpWriteReg(dev, serdesBase | 0x404, 0x28);
    mvOsDelay(10);
    /* DP Clock Select */
    mvOsDelay(1);
    mvPpReadModWriteReg(dev, MV_PP_STACK_REG(stackPort0, 0x090), 0xC, 0x4);
    mvOsDelay(1);
    mvPpReadModWriteReg(dev, MV_PP_STACK_REG(stackPort1, 0x090), 0xC, 0x4);
    /* Deassert GIG MAC Reset */
    mvOsDelay(1);
    mvPpWriteReg(dev, MV_PP_STACK_REG(stackPort0, 0x008), 0xC008);
    mvOsDelay(1);
    mvPpWriteReg(dev, MV_PP_STACK_REG(stackPort1, 0x008), 0xC008);
    /* Disable AN and force link down */
    mvOsDelay(1);
    mvPpWriteReg(dev, MV_PP_STACK_REG(stackPort0, 0x00C), 0x9069);
    mvOsDelay(1);
    mvPpWriteReg(dev, MV_PP_STACK_REG(stackPort1, 0x00C), 0x9069);

    mvOsDelay(1);
    /*
     * PP dev0 stacking port 27 is wired to dev1 stacking port 24.
     * PP dev0 stacking port 26 is wired to dev1 stacking port 25.
     * Force link down on dev0 port 26 and dev1 port 25.
     */
    if (dev == MV_PP_DEV0)
    {
        mvPpStackPortForceLinkDown(dev, stackPort0);
        mvPpStackPortForceLinkUp  (dev, stackPort1);
    }
    else
    {
        mvPpStackPortForceLinkDown(dev, stackPort1);
        mvPpStackPortForceLinkUp  (dev, stackPort0);
    }

    /* Disable Gig Pcs */
    mvOsDelay(1); mvPpWriteReg(dev, 0xA800008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA802008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA804008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA800408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA802408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA804408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA800808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA802808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA804808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA800C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA802C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA804C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA801008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA803008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA805008, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA801408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA803408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA805408, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA801808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA803808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA805808, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA801C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA803C08, 0xC000);
    mvOsDelay(1); mvPpWriteReg(dev, 0xA805C08, 0xC000);
    /* Restart Autoneg */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x13200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x23200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x33200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x43200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x53200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x63200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x73200);
    /* Restart Autoneg */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x83200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0x93200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xA3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xB3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xC3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xD3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xE3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004054, 0xF3200);
    /* Restart Autoneg */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x3200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x13200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x23200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x33200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x43200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x53200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x63200);
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004054, 0x73200);
    /* Port[11..0], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004100, 0x2D75FF00);
    /* Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004100, 0x2D75FF00);
    /* Port[11..0], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004104, 0x44442222);
    /* Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004104, 0x44442222);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4804104, 0x33335555);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5804104, 0x33335555);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4004108, 0xE000E000);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5004108, 0xE000E000);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4804108, 0x0);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5804108, 0x0);
    /* Act_ & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x400410C, 0x6000);
    /* Act_ & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x500410C, 0x6000);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x480410C, 0x0);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(dev, 0x580410C, 0x0);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4804100, 0x11000);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5804100, 0x11000);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1); mvPpWriteReg(dev, 0x5005110, 0x10);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1); mvPpWriteReg(dev, 0x4005110, 0x10);
    /* Device Enable */
    mvOsDelay(1); mvPpReadModWriteReg(dev, 0x58, 0x1, 0x1);

    return MV_OK;
}

/*******************************************************************************
 * mvPpEeprom_DB_XCat2A1_24GE_4GP
 */
MV_STATUS mvPpEeprom_DB_XCat2A1_24GE_4GP(void)
{
    /* CT For_Release _Confi */
    /* File Generated on 24/6/2010 16:55:05 */
    /* 24 QSGMII -- with 88E1340 */
    /* 4x SGMII2.5G -- No PHY */
    /* create device */

    /* set SMI0 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004200, 0x70000);
    /* set SMI1 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004200, 0x70000);
    /* Pipe0 access DFX Setting- Pipe Select */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x308F8004, 0x1);
    /* PCL TCAM-DFX Setting */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x30804050, 0x20003);
    /* RTR TCAM-DFX Setting */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x3080C050, 0x20003);
    /* changed in xCat2, GPP PAD control */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x8F8304, 0x48);
    /* changed in xCat2, SSMII PAD control */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x8F8314, 0x48);
    /* changed in xCat2, RGMII PAD control */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x8F8310, 0x48);
    /* changed in xCat2, LED PAD control */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x8F8300, 0x48);
    /* disable PECL receiver and common_0_PECL_EN=CMOS */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9C, 0x61B0CC3);
    /* Set Extended Control Register */
    /* A1 configuration */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5C, 0x807405);
    /* Power up 5G SERDESs */
    /* Set SERDES ref clock register */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800000, 0x3E80);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800400, 0x3E80);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801000, 0x3E80);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801400, 0x3E80);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802000, 0x3E80);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802400, 0x3E80);
    /* Wait 10mSec */
    mvOsDelay(10);
    /* Deactivate sd_reset */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800004, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800404, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801004, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801404, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802004, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802404, 0x8);
    /* Wait for Calibration done (0x09800008 bit 3) */
    mvOsDelay(10);
    /* Reference Ferquency select = 62.5MHz   ;Use registers bits to control speed configuration */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980020C, 0x800A);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980060C, 0x800A);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980120C, 0x800A);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980160C, 0x800A);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980220C, 0x800A);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980260C, 0x800A);
    /* Transmitter/Reciver Divider force, interpulator force; 1.25G: intpi = 25uA , VCO divided by 4 ; 2.5G: intpi = 25uA , VCO divided by 2  ; 3.125G: intpi = 30uA , VCO divided by 2 ; 3.75G: intpi = 20uA , VCO not divided; 6.25G: intpi = 30uA , VCO not divided; 5.15G: intpi = 25uA , VCO not divided */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800210, 0x4414);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800610, 0x4414);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801210, 0x4414);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801610, 0x4414);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802210, 0x4414);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802610, 0x4414);
    /* Force FbDiv/RfDiv */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800214, 0xA150);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800614, 0xA150);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801214, 0xA150);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801614, 0xA150);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802214, 0xA150);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802614, 0xA150);
    /* Force: PLL Speed, sel_v2i, loadcap_pll,sel_fplres */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800218, 0xBAAB);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800618, 0xBAAB);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801218, 0xBAAB);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801618, 0xBAAB);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802218, 0xBAAB);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802618, 0xBAAB);
    /* icp force */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980021C, 0x882C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980061C, 0x882C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980121C, 0x882C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980161C, 0x882C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980221C, 0x882C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980261C, 0x882C);
    /*  0 = kvco-2 */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x98003CC, 0x2000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x98007CC, 0x2000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x98013CC, 0x2000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x98017CC, 0x2000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x98023CC, 0x2000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x98027CC, 0x2000);
    /* External TX/Rx Impedance changed from 6 to 0 while auto calibration results are used  - based on lab measurments it seems that we need to force the auto imedance calibration values */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980022C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980062C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980122C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980162C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980222C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980262C, 0x0);
    /* Auto KVCO,  PLL is not forced to max speed during power up sequence - */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800230, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800630, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801230, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801630, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802230, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802630, 0x0);
    /* Sampler OS Scale was changed from 5mV/Step to 3.3mV/Step; RX_IMP_VTHIMCAL was chnge from 3 to 0 */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800234, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800634, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801234, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801634, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802234, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802634, 0x4000);
    /* Use value wiritten to register for process calibration instead of th eauto calibration; Select process from register */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980023C, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980063C, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980123C, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980163C, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980223C, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980263C, 0x18);
    /* DCC should be dissabled at baud 3.125 and below = 8060 */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800250, 0xA0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800650, 0xA0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801250, 0xA0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801650, 0xA0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802250, 0xA0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802650, 0xA0C0);
    mvOsDelay(5);
    /* DCC should be dissabled at baud 3.125 and below = 8060 */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800250, 0xA060);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800650, 0xA060);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801250, 0xA060);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801650, 0xA060);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802250, 0xA060);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802650, 0xA060);
    /* PE Setting */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800254, 0x750C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800654, 0x750C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801254, 0x750C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801654, 0x750C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802254, 0x750C);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802654, 0x750C);
    /* PE Type */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800258, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800658, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801258, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801658, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802258, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802658, 0x0);
    /* selmupi/mupf - low value for lower baud */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980027C, 0x90AA);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980067C, 0x90AA);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980127C, 0x90AA);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980167C, 0x90AA);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980227C, 0x90AA);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980267C, 0x90AA);
    /* DTL_FLOOP_EN = Dis */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800280, 0x800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800680, 0x800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801280, 0x800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801680, 0x800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802280, 0x800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802680, 0x800);
    /* FFE Setting */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980028C, 0x363);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980068C, 0x363);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980128C, 0x363);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980168C, 0x363);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980228C, 0x363);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980268C, 0x363);
    /* Slicer Enable; Tx  Imp was changed from 50ohm to 43ohm */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980035C, 0x423F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980075C, 0x423F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980135C, 0x423F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980175C, 0x423F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980235C, 0x423F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980275C, 0x423F);
    /* Not need to be configure - Same as default */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800364, 0x5555);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800764, 0x5555);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801364, 0x5555);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801764, 0x5555);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802364, 0x5555);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802764, 0x5555);
    /* Disable ana_clk_det */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980036C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980076C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980136C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980176C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980236C, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980276C, 0x0);
    /* Configure rx_imp_vthimpcal to 0x0 (default value = 0x3); Configure Sampler_os_scale to 3.3mV/step (default value = 5mV/step) */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800234, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800634, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801234, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801634, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802234, 0x4000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802634, 0x4000);
    /* Configure IMP_VTHIMPCAL to 56.7ohm (default value = 53.3 ohm); Configure cal_os_ph_rd to 0x60 (default value = 0x0); Configure Cal_rxclkalign90_ext to use an external ovride value */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800228, 0xE0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800628, 0xE0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801228, 0xE0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801628, 0xE0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802228, 0xE0C0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802628, 0xE0C0);
    /* Reset dtl_rx ; Enable ana_clk_det */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980036C, 0x8040);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980076C, 0x8040);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980136C, 0x8040);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980176C, 0x8040);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980236C, 0x8040);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980276C, 0x8040);
    /* Un reset dtl_rx */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980036C, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980076C, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980136C, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980176C, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980236C, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x980276C, 0x8000);
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800224, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800624, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801224, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801624, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802224, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802624, 0x0);
    /* CAL Start */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800224, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800624, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801224, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801624, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802224, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802624, 0x8000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800224, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800624, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801224, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801624, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802224, 0x0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802624, 0x0);
    /* Wait for RxClk_x2 */
    mvOsDelay(10);
    /* Set RxInit to 0x1 (remember that bit 3 is already set to 0x1) */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800004, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800404, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801004, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801404, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802004, 0x18);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802404, 0x18);
    /* Wait for p_clk = 1 and p_clk = 0 */
    mvOsDelay(10);
    /* Set RxInit to 0x0 */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800004, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800404, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801004, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801404, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802004, 0x8);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802404, 0x8);
    /* Wait for ALL PHY_RDY = 1 (0x09800008 bit 0) */
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800004, 0x28);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9800404, 0x28);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801004, 0x28);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9801404, 0x28);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802004, 0x28);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9802404, 0x28);
    mvOsDelay(10);
    /* Deassert GIG MAC Reset */
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA806008, 0xC008);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA806408, 0xC008);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA806808, 0xC008);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA806C08, 0xC008);
    /* Disable AN */
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA80600C, 0x9068);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA80640C, 0x9068);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA80680C, 0x9068);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA806C0C, 0x9068);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA800008, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA801008, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA802008, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA803008, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA804008, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA805008, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA800408, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA801408, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA802408, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA803408, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA804408, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA805408, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA800808, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA801808, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA802808, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA803808, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA804808, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA805808, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA800C08, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA801C08, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA802C08, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA803C08, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA804C08, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0xA805C08, 0xC009);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C400FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x144000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C400FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1641CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C400FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2242299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C400FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1641D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C400FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3042800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C400FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2E42001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A40010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C44018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A40003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C40002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x49140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C500FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x145000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C500FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1651CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C500FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2252299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C500FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1651D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C500FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3052800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C500FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2E52001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A50010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C54018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A50003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C50002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x59140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C600FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x146000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C600FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1661CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C600FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2262299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C600FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1661D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C600FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3062800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C600FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2E62001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A60010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C64018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A60003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C60002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x69140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C700FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x147000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C700FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1671CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C700FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2272299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C700FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1671D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C700FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3072800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C700FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2E72001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A70010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C74018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A70003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C70002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x79140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C800FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x148000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C800FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1681CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C800FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2282299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C800FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1681D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C800FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3082800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C800FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2E82001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A80010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C84018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A80003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C80002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x89140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C900FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x149000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C900FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1691CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C900FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2292299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C900FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x1691D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C900FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3092800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C900FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2E92001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A90010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C94018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A90003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3C90002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3A90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x99140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x14A000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16A1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x22A2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16A1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x30A2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2EA2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AA0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CA4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AA0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CA0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0xA9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x14B000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16B1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x22B2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16B1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x30B2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2EB2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AB0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CB4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AB0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CB0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0xB9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x14C000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16C1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x22C2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16C1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x30C2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2EC2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AC0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CC4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AC0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CC0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0xC9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x14D000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16D1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x22D2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16D1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x30D2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2ED2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AD0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CD4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AD0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CD0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0xD9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x14E000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16E1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x22E2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16E1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x30E2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2EE2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AE0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CE4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AE0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CE0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0xE9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x14F000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16F1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x22F2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x16F1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x30F2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2EF2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AF0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CF4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AF0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3CF0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x3AF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004054, 0xF9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C400FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x144000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C400FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1641CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C400FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2242299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C400FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1641D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C400FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3042800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C400FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2E42001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A40010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C44018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A40003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C40002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C40000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x49140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C500FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x145000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C500FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1651CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C500FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2252299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C500FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1651D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C500FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3052800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C500FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2E52001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A50010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C54018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A50003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C50002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C50000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x59140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C600FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x146000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C600FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1661CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C600FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2262299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C600FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1661D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C600FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3062800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C600FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2E62001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A60010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C64018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A60003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C60002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C60000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x69140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C700FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x147000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C700FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1671CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C700FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2272299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C700FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1671D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C700FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3072800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C700FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2E72001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A70010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C74018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A70003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C70002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C70000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x79140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C800FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x148000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C800FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1681CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C800FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2282299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C800FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1681D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C800FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3082800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C800FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2E82001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A80010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C84018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A80003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C80002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C80000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x89140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C900FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x149000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C900FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1691CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C900FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2292299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C900FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x1691D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C900FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3092800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C900FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2E92001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A90010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C94018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A90003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3C90002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3A90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2C90000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x99140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x14A000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16A1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x22A2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16A1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x30A2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2EA2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AA0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CA4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AA0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CA0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CA0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0xA9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x14B000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16B1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x22B2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16B1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x30B2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2EB2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AB0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CB4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AB0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CB0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CB0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0xB9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x14C000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16C1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x22C2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16C1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x30C2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2EC2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AC0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CC4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AC0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CC0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CC0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0xC9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x14D000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16D1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x22D2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16D1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x30D2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2ED2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AD0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CD4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AD0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CD0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CD0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0xD9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x14E000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16E1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x22E2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16E1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x30E2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2EE2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AE0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CE4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AE0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CE0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CE0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0xE9140);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x14F000F);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16F1CF0);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x22F2299);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF00FD);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x16F1D70);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x30F2800);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF00FF);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2EF2001);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AF0010);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CF4018);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AF0003);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3CF0002);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x3AF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0x2CF0000);
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004054, 0xF9140);
    /*  Port[11..0], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004100, 0x337BFF00);
    /*  Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004100, 0x337BFF00);
    /*  Port[11..0], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4004104, 0x404014B4);
    /*  Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5004104, 0x404014B4);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4804104, 0x55552222);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5804104, 0x55552222);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4804108, 0x0);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5804108, 0x0);
    /*  Act_ & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x400410C, 0xE000);
    /*  Act_ & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x500410C, 0xE000);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x480410C, 0x0);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x580410C, 0x0);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4804100, 0x11000);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5804100, 0x11000);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x5005110, 0x10);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x4005110, 0x10);
    /* RefClk will be PECL level; COMMON_0_PU=1; re-writing this reg. */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x9C, 0x61B0EE3);
    /* DevEn */
    mvOsDelay(1); mvPpWriteReg(0x0, 0x58, 0x4184003);

    return MV_OK;
}

MV_STATUS simulate_PP_EEPROM_4122_24GE_PHY_A0(void)
{
    /*  System : DB-xCat-24GE-4GP */
    /*  Core clock : 222MHz */
    /*  Serdes refrence clock : 125MHz External */
    /*  NP Ports mode:QSGMII */
    /*  SP24 Mode:2Lane Baud rate:6.25G Reach ability:LR */
    /*  SP25 Mode:2Lane Baud rate:6.25G Reach ability:LR */
    /*  SP26 Mode:4Lane Baud rate:3.125G Reach ability:LR */
    /*  SP27 Mode:4Lane Baud rate:3.125G Reach ability:LR */

    /*  set SMI0 fast_mdc to div/64 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004200, 0x00070000);
    /*  set SMI1 fast_mdc to div/64 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004200, 0x00070000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000aaaa);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000aaaa);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000aaaa);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000aaaa);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000aaaa);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000aaaa);
    /*  ------------- 88E1340 Phy configuration on LMS0 ------------- */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c400fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02242299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c40000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03448040);
    /*  ------------- LMS0 Phy configured: 0 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c500fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02252299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c50000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03458040);
    /*  ------------- LMS0 Phy configured: 1 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c600fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02262299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c60000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03468040);
    /*  ------------- LMS0 Phy configured: 2 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c700fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02272299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c70000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03478040);
    /*  ------------- LMS0 Phy configured: 3 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c800fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02282299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c80000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03488040);
    /*  ------------- LMS0 Phy configured: 4 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c900fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02292299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c90000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03498040);
    /*  ------------- LMS0 Phy configured: 5 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ca00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022a2299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ca0000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x034a8040);
    /*  ------------- LMS0 Phy configured: 6 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cb00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022b2299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cb0000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x034b8040);
    /*  ------------- LMS0 Phy configured: 7 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cc00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022c2299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cc0000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x034c8040);
    /*  ------------- LMS0 Phy configured: 8 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cd00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022d2299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cd0000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x034d8040);
    /*  ------------- LMS0 Phy configured: 9 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ce00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022e2299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ce0000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x034e8040);
    /*  ------------- LMS0 Phy configured: 10 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cf00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022f2299);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cf0000);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x034f8040);
    /*  ------------- LMS0 Phy configured: 11 */
    /*  ------------- 88E1340 Phy configuration on LMS1 ------------- */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c400fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02242299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c40000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03448040);
    /*  ------------- LMS1 Phy configured: 0 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c500fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02252299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c50000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03458040);
    /*  ------------- LMS1 Phy configured: 1 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c600fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02262299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c60000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03468040);
    /*  ------------- LMS1 Phy configured: 2 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c700fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02272299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c70000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03478040);
    /*  ------------- LMS1 Phy configured: 3 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c800fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02282299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c80000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03488040);
    /*  ------------- LMS1 Phy configured: 4 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c900fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02292299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c90000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03498040);
    /*  ------------- LMS1 Phy configured: 5 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ca00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022a2299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ca0000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x034a8040);
    /*  ------------- LMS1 Phy configured: 6 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cb00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022b2299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cb0000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x034b8040);
    /*  ------------- LMS1 Phy configured: 7 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cc00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022c2299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cc0000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x034c8040);
    /*  ------------- LMS1 Phy configured: 8 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cd00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022d2299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cd0000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x034d8040);
    /*  ------------- LMS1 Phy configured: 9 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ce00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022e2299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ce0000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x034e8040);
    /*  ------------- LMS1 Phy configured: 10 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cf00fd);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022f2299);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cf0000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x034f8040);
    /*  ------------- LMS1 Phy configured: 11 */
    /*  set P22,23 to dual media */
    mvPpWriteReg(MV_PP_DEV0, 0x05804034, 0x00000030);
    /*  Buffer managment sharing */
    mvPpWriteReg(MV_PP_DEV0, 0x03000000, 0x2fd7faff);
    /*  ANA_GRP configuration (SE or differential select) */
    mvPpWriteReg(MV_PP_DEV0, 0x0000009c, 0x071b8ce3);
    /*  Device configuration register bits 0-22 */
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7d840000);
    /*  HGS device configuration SGMII or HGS */
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7d840000);
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7d840000);
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7d840000);
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7d840000);
    /*  extended global configuration2 RCVD clock - disable, 2nd RCVD clock - disable, output clock - disable */
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    /*  extended global configuration2 XAUI/RXAUI select, SP lane 0 or 3 swap - disable */
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    /*  Configuring network SD to 5Gbps  */
    /*  Pre Power-Up Serdes No.0  */
    mvPpWriteReg(MV_PP_DEV0, 0x09800000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);

    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Network Serdes Internal registers to 5GBps No.0  */
    mvPpWriteReg(MV_PP_DEV0, 0x0980020c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09800210, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09800214, 0x0000a150);
    mvPpWriteReg(MV_PP_DEV0, 0x09800218, 0x0000baab);
    mvPpWriteReg(MV_PP_DEV0, 0x0980021c, 0x0000882c);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09800230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980023c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09800250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09800254, 0x0000770c);
    mvPpWriteReg(MV_PP_DEV0, 0x09800258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980027c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09800280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980028c, 0x00000268);
    mvPpWriteReg(MV_PP_DEV0, 0x0980035c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00008000);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00000000);
    /*  Delay 5mS ----------------  */
    mvOsDelay(50);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.0  */
    mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000028);
    /*  Pre Power-Up Serdes No.1  */
    mvPpWriteReg(MV_PP_DEV0, 0x09800400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Network Serdes Internal registers to 5GBps No.1  */
    mvPpWriteReg(MV_PP_DEV0, 0x0980060c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09800610, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09800614, 0x0000a150);
    mvPpWriteReg(MV_PP_DEV0, 0x09800618, 0x0000baab);
    mvPpWriteReg(MV_PP_DEV0, 0x0980061c, 0x0000882c);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09800630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980063c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09800650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09800654, 0x0000770c);
    mvPpWriteReg(MV_PP_DEV0, 0x09800658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980067c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09800680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980068c, 0x00000268);
    mvPpWriteReg(MV_PP_DEV0, 0x0980075c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00008000);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00000000);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.1  */
    mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000028);
    /*  Pre Power-Up Serdes No.2  */
    mvPpWriteReg(MV_PP_DEV0, 0x09801000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Network Serdes Internal registers to 5GBps No.2  */
    mvPpWriteReg(MV_PP_DEV0, 0x0980120c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09801210, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09801214, 0x0000a150);
    mvPpWriteReg(MV_PP_DEV0, 0x09801218, 0x0000baab);
    mvPpWriteReg(MV_PP_DEV0, 0x0980121c, 0x0000882c);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09801230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980123c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09801250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09801254, 0x0000770c);
    mvPpWriteReg(MV_PP_DEV0, 0x09801258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980127c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09801280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980128c, 0x00000268);
    mvPpWriteReg(MV_PP_DEV0, 0x0980135c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00008000);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00000000);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.2  */
    mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000028);
    /*  Pre Power-Up Serdes No.3  */
    mvPpWriteReg(MV_PP_DEV0, 0x09801400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Network Serdes Internal registers to 5GBps No.3  */
    mvPpWriteReg(MV_PP_DEV0, 0x0980160c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09801610, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09801614, 0x0000a150);
    mvPpWriteReg(MV_PP_DEV0, 0x09801618, 0x0000baab);
    mvPpWriteReg(MV_PP_DEV0, 0x0980161c, 0x0000882c);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09801630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980163c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09801650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09801654, 0x0000770c);
    mvPpWriteReg(MV_PP_DEV0, 0x09801658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980167c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09801680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980168c, 0x00000268);
    mvPpWriteReg(MV_PP_DEV0, 0x0980175c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00008000);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00000000);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.3  */
    mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000028);
    /*  Pre Power-Up Serdes No.4  */
    mvPpWriteReg(MV_PP_DEV0, 0x09802000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Network Serdes Internal registers to 5GBps No.4  */
    mvPpWriteReg(MV_PP_DEV0, 0x0980220c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09802210, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09802214, 0x0000a150);
    mvPpWriteReg(MV_PP_DEV0, 0x09802218, 0x0000baab);
    mvPpWriteReg(MV_PP_DEV0, 0x0980221c, 0x0000882c);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09802230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980223c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09802250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09802254, 0x0000770c);
    mvPpWriteReg(MV_PP_DEV0, 0x09802258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980227c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09802280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980228c, 0x00000268);
    mvPpWriteReg(MV_PP_DEV0, 0x0980235c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980236c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980236c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980236c, 0x00008000);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00000000);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.4  */
    mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000028);
    /*  Pre Power-Up Serdes No.5  */
    mvPpWriteReg(MV_PP_DEV0, 0x09802400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Network Serdes Internal registers to 5GBps No.5  */
    mvPpWriteReg(MV_PP_DEV0, 0x0980260c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09802610, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09802614, 0x0000a150);
    mvPpWriteReg(MV_PP_DEV0, 0x09802618, 0x0000baab);
    mvPpWriteReg(MV_PP_DEV0, 0x0980261c, 0x0000882c);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09802630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980263c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09802650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09802654, 0x0000770c);
    mvPpWriteReg(MV_PP_DEV0, 0x09802658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980267c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09802680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980268c, 0x00000268);
    mvPpWriteReg(MV_PP_DEV0, 0x0980275c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980276c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980276c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980276c, 0x00008000);
    /*  ----------- Delay 10mS ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00000000);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.5  */
    mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000028);
    /*  NP ports de-assert MAC reset  */
    mvPpWriteReg(MV_PP_DEV0, 0x0a800008, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800408, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800808, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800c08, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801008, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801408, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801808, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801c08, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802008, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802408, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802808, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802c08, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803008, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803408, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803808, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803c08, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804008, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804408, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804808, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804c08, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805008, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805408, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805808, 0x0000c008);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805c08, 0x0000c008);
    /*  SP24 ---------------- Serdes configuration -----------------  */
    /*  Pre Power-Up Serdes No.0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09803000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.0 to 6.25Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980320c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09803210, 0x00004415);
    mvPpWriteReg(MV_PP_DEV0, 0x09803214, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09803218, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980321c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09803230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980323c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09803250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09803254, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09803258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980327c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09803280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980328c, 0x0000037f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980335c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980336c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980336c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980336c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09803224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000028);
    /*  Pre Power-Up Serdes No.1 */
    mvPpWriteReg(MV_PP_DEV0, 0x09803400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.1 to 6.25Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980360c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09803610, 0x00004415);
    mvPpWriteReg(MV_PP_DEV0, 0x09803614, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09803618, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980361c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09803630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980363c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09803650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09803654, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09803658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980367c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09803680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980368c, 0x0000037f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980375c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980376c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980376c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980376c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09803624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09803624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.1 */
    mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000028);
    /*  SP25 ---------------- Serdes configuration -----------------  */
    /*  Pre Power-Up Serdes No.0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09805000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.0 to 6.25Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980520c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09805210, 0x00004415);
    mvPpWriteReg(MV_PP_DEV0, 0x09805214, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09805218, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980521c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09805230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980523c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09805250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09805254, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09805258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980527c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09805280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980528c, 0x0000037f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980535c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980536c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980536c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980536c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09805224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000028);
    /*  Pre Power-Up Serdes No.1 */
    mvPpWriteReg(MV_PP_DEV0, 0x09805400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.1 to 6.25Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980560c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09805610, 0x00004415);
    mvPpWriteReg(MV_PP_DEV0, 0x09805614, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09805618, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980561c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09805630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980563c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09805650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09805654, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09805658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980567c, 0x000090aa);
    mvPpWriteReg(MV_PP_DEV0, 0x09805680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980568c, 0x0000037f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980575c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980576c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980576c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980576c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09805624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09805624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.1 */
    mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000028);
    /*  SP26 ---------------- Serdes configuration -----------------  */
    /*  Pre Power-Up Serdes No.0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09807000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.0 to 3.125Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980720c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09807210, 0x00005515);
    mvPpWriteReg(MV_PP_DEV0, 0x09807214, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09807218, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980721c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09807230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980723c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09807250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09807254, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09807258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980727c, 0x0000905a);
    mvPpWriteReg(MV_PP_DEV0, 0x09807280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980728c, 0x0000037d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980735c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980736c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980736c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980736c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09807224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000028);
    /*  Pre Power-Up Serdes No.1 */
    mvPpWriteReg(MV_PP_DEV0, 0x09807400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.1 to 3.125Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980760c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09807610, 0x00005515);
    mvPpWriteReg(MV_PP_DEV0, 0x09807614, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09807618, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980761c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09807630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980763c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09807650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09807654, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09807658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980767c, 0x0000905a);
    mvPpWriteReg(MV_PP_DEV0, 0x09807680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980768c, 0x0000037d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980775c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980776c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980776c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980776c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09807624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09807624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.1 */
    mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000028);
    /*  Pre Power-Up Serdes No.2 */
    mvPpWriteReg(MV_PP_DEV0, 0x09808000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.2 to 3.125Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980820c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09808210, 0x00005515);
    mvPpWriteReg(MV_PP_DEV0, 0x09808214, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09808218, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980821c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09808230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980823c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09808250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09808254, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09808258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980827c, 0x0000905a);
    mvPpWriteReg(MV_PP_DEV0, 0x09808280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980828c, 0x0000037d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980835c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980836c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980836c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980836c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09808224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.2 */
    mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000028);
    /*  Pre Power-Up Serdes No.3 */
    mvPpWriteReg(MV_PP_DEV0, 0x09808400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.3 to 3.125Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980860c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09808610, 0x00005515);
    mvPpWriteReg(MV_PP_DEV0, 0x09808614, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09808618, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980861c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09808630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980863c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09808650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09808654, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09808658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980867c, 0x0000905a);
    mvPpWriteReg(MV_PP_DEV0, 0x09808680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980868c, 0x0000037d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980875c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980876c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980876c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980876c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09808624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09808624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.3 */
    mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000028);
    /*  SP27 ---------------- Serdes configuration -----------------  */
    /*  Pre Power-Up Serdes No.0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09809000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.0 to 3.125Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980920c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09809210, 0x00005515);
    mvPpWriteReg(MV_PP_DEV0, 0x09809214, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09809218, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980921c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09809230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980923c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09809250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09809254, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09809258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980927c, 0x0000905a);
    mvPpWriteReg(MV_PP_DEV0, 0x09809280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980928c, 0x0000037d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980935c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980936c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980936c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980936c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09809224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000028);
    /*  Pre Power-Up Serdes No.1 */
    mvPpWriteReg(MV_PP_DEV0, 0x09809400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.1 to 3.125Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980960c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x09809610, 0x00005515);
    mvPpWriteReg(MV_PP_DEV0, 0x09809614, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x09809618, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980961c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09809630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980963c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09809650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x09809654, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x09809658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980967c, 0x0000905a);
    mvPpWriteReg(MV_PP_DEV0, 0x09809680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980968c, 0x0000037d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980975c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980976c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980976c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980976c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x09809624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09809624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.1 */
    mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000028);
    /*  Pre Power-Up Serdes No.2 */
    mvPpWriteReg(MV_PP_DEV0, 0x0980a000, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.2 to 3.125Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980a20c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a210, 0x00005515);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a214, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a218, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a21c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a230, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a23c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a250, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a254, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a27c, 0x0000905a);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a280, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a28c, 0x0000037d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a35c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a36c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a234, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a228, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a36c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a36c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.2 */
    mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000028);
    /*  Pre Power-Up Serdes No.3 */
    mvPpWriteReg(MV_PP_DEV0, 0x0980a400, 0x00003e80);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000008);
    /*  Delay 5mS ----------------  */
    mvOsDelay(5);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Configuring Internal registers of lane No.3 to 3.125Gbps */
    mvPpWriteReg(MV_PP_DEV0, 0x0980a60c, 0x00008001);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a610, 0x00005515);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a614, 0x0000a164);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a618, 0x0000fbaa);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a61c, 0x00008720);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a630, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a63c, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a650, 0x0000a060);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a654, 0x00003f2d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a67c, 0x0000905a);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a680, 0x00004800);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a68c, 0x0000037d);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a75c, 0x0000423f);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a76c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a634, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a628, 0x0000e0a0);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a76c, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a76c, 0x00008000);
    /*  ----------- Delay 10mS  ----------- */
    mvOsDelay(10);
    mvPpWriteReg(MV_PP_DEV0,0x0980a624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /*  Post Power-Up Serdes No.3 */
    mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000018);
    /*  Add minimum 16 core clock delay -----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000028);
    /*  Stack SP24 XPCS lane configuration -----------------  */
    mvPpWriteReg(MV_PP_DEV0, 0x08806254, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x08806298, 0x00000900);
    mvPpWriteReg(MV_PP_DEV0, 0x088062dc, 0x00001200);
    mvPpWriteReg(MV_PP_DEV0, 0x08806320, 0x00001b00);
    /*  Stack SP25 XPCS lane configuration -----------------  */
    mvPpWriteReg(MV_PP_DEV0, 0x08806654, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x08806698, 0x00000900);
    mvPpWriteReg(MV_PP_DEV0, 0x088066dc, 0x00001200);
    mvPpWriteReg(MV_PP_DEV0, 0x08806720, 0x00001b00);
    /*  Stack SP26 XPCS lane configuration -----------------  */
    mvPpWriteReg(MV_PP_DEV0, 0x08806a54, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x08806a98, 0x00000900);
    mvPpWriteReg(MV_PP_DEV0, 0x08806adc, 0x00001200);
    mvPpWriteReg(MV_PP_DEV0, 0x08806b20, 0x00001b00);
    /*  Stack SP27 XPCS lane configuration -----------------  */
    mvPpWriteReg(MV_PP_DEV0, 0x08806e54, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x08806e98, 0x00000900);
    mvPpWriteReg(MV_PP_DEV0, 0x08806edc, 0x00001200);
    mvPpWriteReg(MV_PP_DEV0, 0x08806f20, 0x00001b00);
    /*  PCS de-assert XG port 24 reset & lane configuration ---------------- */
    mvPpWriteReg(MV_PP_DEV0, 0x08806200, 0x00000027);
    mvPpWriteReg(MV_PP_DEV0, 0x08806204, 0x000000c0);
    /*  PCS de-assert XG port 25 reset & lane configuration ---------------- */
    mvPpWriteReg(MV_PP_DEV0, 0x08806600, 0x00000027);
    mvPpWriteReg(MV_PP_DEV0, 0x08806604, 0x000000c0);
    /*  PCS de-assert XG port 26 reset & lane configuration ---------------- */
    mvPpWriteReg(MV_PP_DEV0, 0x08806a00, 0x00000047);
    mvPpWriteReg(MV_PP_DEV0, 0x08806a04, 0x000000c0);
    /*  PCS de-assert XG port 27 reset & lane configuration ---------------- */
    mvPpWriteReg(MV_PP_DEV0, 0x08806e00, 0x00000047);
    mvPpWriteReg(MV_PP_DEV0, 0x08806e04, 0x000000c0);
    /*  De-assert XG MAC Reset Stacking port 24  --------------  */
    mvPpWriteReg(MV_PP_DEV0, 0x08806000, 0x00002403);
    mvPpWriteReg(MV_PP_DEV0, 0x08806004, 0x000002f9);
    mvPpWriteReg(MV_PP_DEV0, 0x08806008, 0x00003818);
    mvPpWriteReg(MV_PP_DEV0, 0x0880601c, 0x00002000);
    /*  De-assert XG MAC Reset  Stacking port 25  --------------  */
    mvPpWriteReg(MV_PP_DEV0, 0x08806400, 0x00002403);
    mvPpWriteReg(MV_PP_DEV0, 0x08806404, 0x000002f9);
    mvPpWriteReg(MV_PP_DEV0, 0x08806408, 0x00003819);
    mvPpWriteReg(MV_PP_DEV0, 0x0880641c, 0x00002000);
    /*  De-assert XG MAC Reset Stacking port 26  --------------  */
    mvPpWriteReg(MV_PP_DEV0, 0x08806800, 0x00002403);
    mvPpWriteReg(MV_PP_DEV0, 0x08806804, 0x000002f9);
    mvPpWriteReg(MV_PP_DEV0, 0x08806808, 0x0000381a);
    mvPpWriteReg(MV_PP_DEV0, 0x0880681c, 0x00002000);
    /*  De-assert XG MAC Reset Stacking port 27  --------------  */
    mvPpWriteReg(MV_PP_DEV0, 0x08806c00, 0x00002403);
    mvPpWriteReg(MV_PP_DEV0, 0x08806c04, 0x000002f9);
    mvPpWriteReg(MV_PP_DEV0, 0x08806c08, 0x0000381b);
    mvPpWriteReg(MV_PP_DEV0, 0x08806c1c, 0x00002000);
    /*  ----------- Led stream configuration for Dragonite on LMS0----------- */
    /*  Port[11..0], Led stream ordered by port & Link effect 'on' */
    /*  Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvPpWriteReg(MV_PP_DEV0, 0x04004100, 0x337bff00);
    mvPpWriteReg(MV_PP_DEV0, 0x05004100, 0x337bff00);
    mvPpWriteReg(MV_PP_DEV0, 0x04004104, 0x00004343);
    mvPpWriteReg(MV_PP_DEV0, 0x04004108, 0x00001000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004104, 0x404014b4);
    /*  full duplex green/yellow, for Ports [23..0] */
    mvPpWriteReg(MV_PP_DEV0, 0x04804104, 0x00003333);
    mvPpWriteReg(MV_PP_DEV0, 0x05804104, 0x55552222);
    /*  FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvPpWriteReg(MV_PP_DEV0, 0x04804108, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x05804108, 0x00000000);
    /*  ~Act & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvPpWriteReg(MV_PP_DEV0, 0x0400410c, 0x0000e000);
    mvPpWriteReg(MV_PP_DEV0, 0x0500410c, 0x0000e000);
    /*  HD ; Class manipulation - Class[5], Port[23..0] */
    mvPpWriteReg(MV_PP_DEV0, 0x0480410c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0580410c, 0x00000000);
    /*  PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvPpWriteReg(MV_PP_DEV0, 0x04804100, 0x00011000);
    mvPpWriteReg(MV_PP_DEV0, 0x05804100, 0x00011000);
    /*  LMS0,1 HGS debug leds force class link */
    mvPpWriteReg(MV_PP_DEV0, 0x05005110, 0x00000010);
    mvPpWriteReg(MV_PP_DEV0, 0x04005110, 0x00000010);
    /*  ---------- Device enable ----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x00000058, 0x04184003);
    /*  ---------- End of EEprom ----------  */

    return MV_OK;
}

/*******************************************************************************
 * simulate_PP_EEPROM_2122_24FE
 */
MV_STATUS simulate_PP_EEPROM_2122_24FE(void)
{
    /*
     * Created By: xCat-A1 configurator Rev2.3
     * System : DB-xCat-24GE-4GP
     * Core clock : 222MHz
     * Serdes refrence clock : 125MHz Internal
     * NP Ports mode:SSMII
     * SP24 Mode:PD Baud rate:6.25G Reach ability:LR
     * SP25 Mode:PD Baud rate:6.25G Reach ability:LR
     * SP26 Mode:PD Baud rate:6.25G Reach ability:LR
     * SP27 Mode:PD Baud rate:6.25G Reach ability:LR
     */
    /* set SMI0 fast_mdc to div/64 16 ports polling */
    mvPpWriteReg(MV_PP_DEV0, 0x04004200, 0x000b0000);
    /* set SMI1 fast_mdc to div/64 8 ports polling */
    mvPpWriteReg(MV_PP_DEV0, 0x05004200, 0x00030000);
    /* Reduce SSMII pad strength to N=2, P=2 */
    mvPpWriteReg(MV_PP_DEV0, 0x00000048, 0x1f848243);
    mvPpWriteReg(MV_PP_DEV0, 0x00000044, 0xf090fc7e);
    mvPpWriteReg(MV_PP_DEV0, 0x00000024, 0x7efc243f);
    /* Stack mode configuration register */
    mvPpWriteReg(MV_PP_DEV0, 0x000000f4, 0x00000000);
    /* ANA_GRP configuration (SE or differential select) */
    mvPpWriteReg(MV_PP_DEV0, 0x0000009c, 0x071b8ce3);
    /* Device configuration register bits 0-22 */
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7e840000);
    mvPpWriteReg(MV_PP_DEV0, 0x0000009c, 0x061b0cc3);
    /* HGS device configuration SGMII or HGS */
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7e840000);
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7e840000);
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7e840000);
    mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7e840000);
    /* extended global configuration2 RCVD clock - disable, 2nd RCVD clock - disable, output clock - disable */
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    /* extended global configuration2 XAUI/RXAUI select, SP lane 0 or 3 swap - disable */
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    /* Configure SMI0 to 16 ports polling.  */
    mvPpWriteReg(MV_PP_DEV0, 0x04004200, 0x000b0000);
    /* Configure SMI1 to 8 ports polling.  */
    mvPpWriteReg(MV_PP_DEV0, 0x05004200, 0x00030000);
    /* Configure phy address LMS0 (0-15), LMS1 0-7  */
    mvPpWriteReg(MV_PP_DEV0, 0x04004030, 0x128398a0);
    mvPpWriteReg(MV_PP_DEV0, 0x04004030, 0x12839820);
    mvPpWriteReg(MV_PP_DEV0, 0x04004030, 0x12838820);
    mvPpWriteReg(MV_PP_DEV0, 0x04004030, 0x12818820);
    mvPpWriteReg(MV_PP_DEV0, 0x04004030, 0x12418820);
    mvPpWriteReg(MV_PP_DEV0, 0x04004030, 0x0a418820);
    mvPpWriteReg(MV_PP_DEV0, 0x04804030, 0x00000006);
    mvPpWriteReg(MV_PP_DEV0, 0x04804030, 0x000000e6);
    mvPpWriteReg(MV_PP_DEV0, 0x04804030, 0x000020e6);
    mvPpWriteReg(MV_PP_DEV0, 0x04804030, 0x0004a0e6);
    mvPpWriteReg(MV_PP_DEV0, 0x04804030, 0x00a4a0e6);
    mvPpWriteReg(MV_PP_DEV0, 0x04804030, 0x16a4a0e6);
    mvPpWriteReg(MV_PP_DEV0, 0x05004030, 0x128398ac);
    mvPpWriteReg(MV_PP_DEV0, 0x05004030, 0x128399ac);
    mvPpWriteReg(MV_PP_DEV0, 0x05004030, 0x1283b9ac);
    mvPpWriteReg(MV_PP_DEV0, 0x05004030, 0x1287b9ac);
    mvPpWriteReg(MV_PP_DEV0, 0x05004030, 0x1207b9ac);
    mvPpWriteReg(MV_PP_DEV0, 0x05004030, 0x0207b9ac);
    mvPpWriteReg(MV_PP_DEV0, 0x05804030, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x05804030, 0x00000062);
    mvPpWriteReg(MV_PP_DEV0, 0x05804030, 0x00001062);
    mvPpWriteReg(MV_PP_DEV0, 0x05804030, 0x00029062);
    mvPpWriteReg(MV_PP_DEV0, 0x05804030, 0x00629062);
    mvPpWriteReg(MV_PP_DEV0, 0x05804030, 0x0e629062);
    /* disable GPCS on all network ports  */
    mvPpWriteReg(MV_PP_DEV0, 0x0a800000, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800004, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800008, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800048, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800400, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800404, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800408, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800448, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800800, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800804, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800808, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800848, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800c00, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800c04, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800c08, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a800c48, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801000, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801004, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801008, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801048, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801400, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801404, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801408, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801448, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801800, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801804, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801808, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801848, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801c00, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801c04, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801c08, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a801c48, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802000, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802004, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802008, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802048, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802400, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802404, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802408, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802448, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802800, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802804, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802808, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802848, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802c00, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802c04, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802c08, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a802c48, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803000, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803004, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803008, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803048, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803400, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803404, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803408, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803448, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803800, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803804, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803808, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803848, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803c00, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803c04, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803c08, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a803c48, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804000, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804004, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804008, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804048, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804400, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804404, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804408, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804448, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804800, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804804, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804808, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804848, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804c00, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804c04, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804c08, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a804c48, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805000, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805004, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805008, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805048, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805400, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805404, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805408, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805448, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805800, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805804, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805808, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805848, 0x00000300);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805c00, 0x00008be5);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805c04, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805c08, 0x0000c000);
    mvPpWriteReg(MV_PP_DEV0, 0x0a805c48, 0x00000300);
    /* Restart autoNeg in all network ports  */
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00003200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00013200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00023200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00033200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00043200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00053200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00063200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00073200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00083200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00093200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x000a3200);
    mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x000b3200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000c3200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000d3200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000e3200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000f3200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00003200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00013200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00023200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00033200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00043200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00053200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00063200);
    mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00073200);
    /* ----------- Led stream configuration ----------- */
    /* Port[11..0], Led stream ordered by port & Link effect 'on' */
    /* Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvPpWriteReg(MV_PP_DEV0, 0x04004100, 0x2d75ff00);
    mvPpWriteReg(MV_PP_DEV0, 0x05004100, 0x2d75ff00);
    /* Ports [23..0]	G0: 	A - '1' 100Mbs / B - '1'100Mbs / C - 'b' 10Mbs  / D - 'b'10 Mbs */
    mvPpWriteReg(MV_PP_DEV0, 0x04004104, 0x44442222);
    mvPpWriteReg(MV_PP_DEV0, 0x05004104, 0x44442222);
    /* full duplex green/yellow, for Ports [23..0] */
    mvPpWriteReg(MV_PP_DEV0, 0x04804104, 0x33335555);
    mvPpWriteReg(MV_PP_DEV0, 0x05804104, 0x33335555);
    mvPpWriteReg(MV_PP_DEV0, 0x04004108, 0xe000e000);
    mvPpWriteReg(MV_PP_DEV0, 0x05004108, 0xe000e000);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvPpWriteReg(MV_PP_DEV0, 0x04804108, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x05804108, 0x00000000);
    /* ~Act & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvPpWriteReg(MV_PP_DEV0, 0x0400410c, 0x00006000);
    mvPpWriteReg(MV_PP_DEV0, 0x0500410c, 0x00006000);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvPpWriteReg(MV_PP_DEV0, 0x0480410c, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0580410c, 0x00000000);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvPpWriteReg(MV_PP_DEV0, 0x04804100, 0x00011000);
    mvPpWriteReg(MV_PP_DEV0, 0x05804100, 0x00011000);
    /* LMS0,1 HGS debug leds force class link */
    mvPpWriteReg(MV_PP_DEV0, 0x05005110, 0x00000010);
    mvPpWriteReg(MV_PP_DEV0, 0x04005110, 0x00000010);
    /* ---------- Device enable ----------  */
    mvPpWriteReg(MV_PP_DEV0, 0x00000058, 0x04184003);

    return MV_OK;
}

/*******************************************************************************
 * simulate_PP_EEPROM_DB_xCat2A0_BGA_24GE_4GP
 */
MV_STATUS simulate_PP_EEPROM_DB_xCat2A0_BGA_24GE_4GP(void)
{
    /* File Generated on 24/6/2010 16:54:52 */
    /* 24 QSGMII -- with 88E1340 */
    /* 4x SGMII2.5G -- No PHY */
    /* set SMI0 fast_mdc to div/64 */
    mvPpWriteReg(MV_PP_DEV0, 0x04004200, 0x00070000);
    /* set SMI1 fast_mdc to div/64 */
    mvPpWriteReg(MV_PP_DEV0, 0x05004200, 0x00070000);
    /* Pipe0 access DFX Setting- Pipe Select  */
    mvPpWriteReg(MV_PP_DEV0, 0x308F8004, 0x00000001);
    /* PCL TCAM-DFX Setting */
    mvPpWriteReg(MV_PP_DEV0, 0x30804050, 0x00020003);
    /* RTR TCAM-DFX Setting */
    mvPpWriteReg(MV_PP_DEV0, 0x3080C050, 0x00020003);
    /* changed in xCat2, GPP PAD control */
    mvPpWriteReg(MV_PP_DEV0, 0x008F8304, 0x00000048);
    /* changed in xCat2, SSMII PAD control */
    mvPpWriteReg(MV_PP_DEV0, 0x008F8314, 0x00000048);
    /* changed in xCat2, RGMII PAD control */
    mvPpWriteReg(MV_PP_DEV0, 0x008F8310, 0x00000048);
    /* changed in xCat2, LED PAD control        */
    mvPpWriteReg(MV_PP_DEV0, 0x008F8300, 0x00000048);
    /* disable PECL receiver and common_0_PECL_EN=CMOS */
    mvPpWriteReg(MV_PP_DEV0, 0x0000009C, 0x061B0CC3);
    /* Set Extended Control Register */
    mvPpWriteReg(MV_PP_DEV0, 0x0000005C, 0x00C03405);
    /* Power up 5G SERDESs */
    /* Set SERDES ref clock register */
    mvPpWriteReg(MV_PP_DEV0, 0x09800000, 0x00003E80);
    mvPpWriteReg(MV_PP_DEV0, 0x09800400, 0x00003E80);
    mvPpWriteReg(MV_PP_DEV0, 0x09801000, 0x00003E80);
    mvPpWriteReg(MV_PP_DEV0, 0x09801400, 0x00003E80);
    mvPpWriteReg(MV_PP_DEV0, 0x09802000, 0x00003E80);
    mvPpWriteReg(MV_PP_DEV0, 0x09802400, 0x00003E80);
    /* Wait 10mSec */
    mvOsDelay(10);
 /* mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000004);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000005);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000006);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000007);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000009); */
    /* Deactivate sd_reset */
    mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    /* Wait for Calibration done (0x09800008 bit 3) */
    mvOsDelay(10);
 /* mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000004);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000005);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000006);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000007);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000009); */
    /* Reference Ferquency select = 62.5MHz ; Use registers bits to control speed configuration */
    mvPpWriteReg(MV_PP_DEV0, 0x0980020C, 0x0000800A);
    mvPpWriteReg(MV_PP_DEV0, 0x0980060C, 0x0000800A);
    mvPpWriteReg(MV_PP_DEV0, 0x0980120C, 0x0000800A);
    mvPpWriteReg(MV_PP_DEV0, 0x0980160C, 0x0000800A);
    mvPpWriteReg(MV_PP_DEV0, 0x0980220C, 0x0000800A);
    mvPpWriteReg(MV_PP_DEV0, 0x0980260C, 0x0000800A);
    mvOsDelay(1);
    /* Transmitter/Reciver Divider force, interpulator force; 1.25G: intpi = 25uA , VCO divided by 4 ; 2.5G: intpi = 25uA , VCO divided by 2  ; 3.125G: intpi = 30uA , VCO divided by 2 ; 3.75G: intpi = 20uA , VCO not divided; 6.25G: intpi = 30uA , VCO not divided; 5.15G: intpi = 25uA , VCO not divided */
    mvPpWriteReg(MV_PP_DEV0, 0x09800210, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09800610, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09801210, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09801610, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09802210, 0x00004414);
    mvPpWriteReg(MV_PP_DEV0, 0x09802610, 0x00004414);
    mvOsDelay(1);
    /* Force FbDiv/RfDiv */
    mvPpWriteReg(MV_PP_DEV0, 0x09800214, 0x0000A150);
    mvPpWriteReg(MV_PP_DEV0, 0x09800614, 0x0000A150);
    mvPpWriteReg(MV_PP_DEV0, 0x09801214, 0x0000A150);
    mvPpWriteReg(MV_PP_DEV0, 0x09801614, 0x0000A150);
    mvPpWriteReg(MV_PP_DEV0, 0x09802214, 0x0000A150);
    mvPpWriteReg(MV_PP_DEV0, 0x09802614, 0x0000A150);
    mvOsDelay(1);
    /* Force: PLL Speed, sel_v2i, loadcap_pll,sel_fplres */
    mvPpWriteReg(MV_PP_DEV0, 0x09800218, 0x0000BAAB);
    mvPpWriteReg(MV_PP_DEV0, 0x09800618, 0x0000BAAB);
    mvPpWriteReg(MV_PP_DEV0, 0x09801218, 0x0000BAAB);
    mvPpWriteReg(MV_PP_DEV0, 0x09801618, 0x0000BAAB);
    mvPpWriteReg(MV_PP_DEV0, 0x09802218, 0x0000BAAB);
    mvPpWriteReg(MV_PP_DEV0, 0x09802618, 0x0000BAAB);
    mvOsDelay(1);
    /* icp force */
    mvPpWriteReg(MV_PP_DEV0, 0x0980021C, 0x0000882C);
    mvPpWriteReg(MV_PP_DEV0, 0x0980061C, 0x0000882C);
    mvPpWriteReg(MV_PP_DEV0, 0x0980121C, 0x0000882C);
    mvPpWriteReg(MV_PP_DEV0, 0x0980161C, 0x0000882C);
    mvPpWriteReg(MV_PP_DEV0, 0x0980221C, 0x0000882C);
    mvPpWriteReg(MV_PP_DEV0, 0x0980261C, 0x0000882C);
    mvOsDelay(1);
    /*  0 = kvco-2 */
    mvPpWriteReg(MV_PP_DEV0, 0x098003CC, 0x00002000);
    mvPpWriteReg(MV_PP_DEV0, 0x098007CC, 0x00002000);
    mvPpWriteReg(MV_PP_DEV0, 0x098013CC, 0x00002000);
    mvPpWriteReg(MV_PP_DEV0, 0x098017CC, 0x00002000);
    mvPpWriteReg(MV_PP_DEV0, 0x098023CC, 0x00002000);
    mvPpWriteReg(MV_PP_DEV0, 0x098027CC, 0x00002000);
    mvOsDelay(1);
    /* External TX/Rx Impedance changed from 6 to 0 while auto calibration results are used  - based on lab measurments it seems that we need to force the auto imedance calibration values */
    mvPpWriteReg(MV_PP_DEV0, 0x0980022C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980062C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980122C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980162C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980222C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980262C, 0x00000000);
    mvOsDelay(1);
    /* Auto KVCO,  PLL is not forced to max speed during power up sequence -  */
    mvPpWriteReg(MV_PP_DEV0, 0x09800230, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800630, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801230, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801630, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802230, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802630, 0x00000000);
    mvOsDelay(1);
    /* Sampler OS Scale was changed from 5mV/Step to 3.3mV/Step; RX_IMP_VTHIMCAL was chnge from 3 to 0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09800234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802634, 0x00004000);
    mvOsDelay(1);
    /* Use value wiritten to register for process calibration instead of the auto calibration; Select process from register */
    mvPpWriteReg(MV_PP_DEV0, 0x0980023C, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x0980063C, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x0980123C, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x0980163C, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x0980223C, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x0980263C, 0x00000018);
    mvOsDelay(1);
    /* DCC should be dissabled at baud 3.125 and below = 8060 */
    mvPpWriteReg(MV_PP_DEV0, 0x09800250, 0x0000A0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09800650, 0x0000A0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09801250, 0x0000A0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09801650, 0x0000A0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09802250, 0x0000A0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09802650, 0x0000A0C0);
    mvOsDelay(10);
/*    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000004);*/
    /* DCC should be dissabled at baud 3.125 and below = 8060 */
    mvPpWriteReg(MV_PP_DEV0, 0x09800250, 0x0000A060);
    mvPpWriteReg(MV_PP_DEV0, 0x09800650, 0x0000A060);
    mvPpWriteReg(MV_PP_DEV0, 0x09801250, 0x0000A060);
    mvPpWriteReg(MV_PP_DEV0, 0x09801650, 0x0000A060);
    mvPpWriteReg(MV_PP_DEV0, 0x09802250, 0x0000A060);
    mvPpWriteReg(MV_PP_DEV0, 0x09802650, 0x0000A060);
    mvOsDelay(1);
    /* PE Setting */
    mvPpWriteReg(MV_PP_DEV0, 0x09800254, 0x0000750C);
    mvPpWriteReg(MV_PP_DEV0, 0x09800654, 0x0000750C);
    mvPpWriteReg(MV_PP_DEV0, 0x09801254, 0x0000750C);
    mvPpWriteReg(MV_PP_DEV0, 0x09801654, 0x0000750C);
    mvPpWriteReg(MV_PP_DEV0, 0x09802254, 0x0000750C);
    mvPpWriteReg(MV_PP_DEV0, 0x09802654, 0x0000750C);
    mvOsDelay(1);
    /* PE Type */
    mvPpWriteReg(MV_PP_DEV0, 0x09800258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801658, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802258, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802658, 0x00000000);
    mvOsDelay(1);
    /* selmupi/mupf - low value for lower baud */
    mvPpWriteReg(MV_PP_DEV0, 0x0980027C, 0x000090AA);
    mvPpWriteReg(MV_PP_DEV0, 0x0980067C, 0x000090AA);
    mvPpWriteReg(MV_PP_DEV0, 0x0980127C, 0x000090AA);
    mvPpWriteReg(MV_PP_DEV0, 0x0980167C, 0x000090AA);
    mvPpWriteReg(MV_PP_DEV0, 0x0980227C, 0x000090AA);
    mvPpWriteReg(MV_PP_DEV0, 0x0980267C, 0x000090AA);
    mvOsDelay(1);
    /* DTL_FLOOP_EN = Dis */
    mvPpWriteReg(MV_PP_DEV0, 0x09800280, 0x00000800);
    mvPpWriteReg(MV_PP_DEV0, 0x09800680, 0x00000800);
    mvPpWriteReg(MV_PP_DEV0, 0x09801280, 0x00000800);
    mvPpWriteReg(MV_PP_DEV0, 0x09801680, 0x00000800);
    mvPpWriteReg(MV_PP_DEV0, 0x09802280, 0x00000800);
    mvPpWriteReg(MV_PP_DEV0, 0x09802680, 0x00000800);
    mvOsDelay(1);
    /* FFE Setting */
    mvPpWriteReg(MV_PP_DEV0, 0x0980028C, 0x00000363);
    mvPpWriteReg(MV_PP_DEV0, 0x0980068C, 0x00000363);
    mvPpWriteReg(MV_PP_DEV0, 0x0980128C, 0x00000363);
    mvPpWriteReg(MV_PP_DEV0, 0x0980168C, 0x00000363);
    mvPpWriteReg(MV_PP_DEV0, 0x0980228C, 0x00000363);
    mvPpWriteReg(MV_PP_DEV0, 0x0980268C, 0x00000363);
    mvOsDelay(1);
    /* Slicer Enable; Tx  Imp was changed from 50ohm to 43ohm */
    mvPpWriteReg(MV_PP_DEV0, 0x0980035C, 0x0000423F);
    mvPpWriteReg(MV_PP_DEV0, 0x0980075C, 0x0000423F);
    mvPpWriteReg(MV_PP_DEV0, 0x0980135C, 0x0000423F);
    mvPpWriteReg(MV_PP_DEV0, 0x0980175C, 0x0000423F);
    mvPpWriteReg(MV_PP_DEV0, 0x0980235C, 0x0000423F);
    mvPpWriteReg(MV_PP_DEV0, 0x0980275C, 0x0000423F);
    mvOsDelay(1);
    /* Not need to be configure - Same as default  */
    mvPpWriteReg(MV_PP_DEV0, 0x09800364, 0x00005555);
    mvPpWriteReg(MV_PP_DEV0, 0x09800764, 0x00005555);
    mvPpWriteReg(MV_PP_DEV0, 0x09801364, 0x00005555);
    mvPpWriteReg(MV_PP_DEV0, 0x09801764, 0x00005555);
    mvPpWriteReg(MV_PP_DEV0, 0x09802364, 0x00005555);
    mvPpWriteReg(MV_PP_DEV0, 0x09802764, 0x00005555);
    mvOsDelay(1);
    /* Disable ana_clk_det */
    mvPpWriteReg(MV_PP_DEV0, 0x0980036C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980076C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980136C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980176C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980236C, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980276C, 0x00000000);
    mvOsDelay(1);
    /* Configure rx_imp_vthimpcal to 0x0 (default value = 0x3); Configure Sampler_os_scale to 3.3mV/step (default value = 5mV/step) */
    mvPpWriteReg(MV_PP_DEV0, 0x09800234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801634, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802234, 0x00004000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802634, 0x00004000);
    mvOsDelay(1);
    /* Configure IMP_VTHIMPCAL to 56.7ohm (default value = 53.3 ohm); Configure cal_os_ph_rd to 0x60 (default value = 0x0); Configure Cal_rxclkalign90_ext to use an external ovride value */
    mvPpWriteReg(MV_PP_DEV0, 0x09800228, 0x0000E0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09800628, 0x0000E0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09801228, 0x0000E0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09801628, 0x0000E0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09802228, 0x0000E0C0);
    mvPpWriteReg(MV_PP_DEV0, 0x09802628, 0x0000E0C0);
    mvOsDelay(1);
    /* Reset dtl_rx ; Enable ana_clk_det */
    mvPpWriteReg(MV_PP_DEV0, 0x0980036C, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980076C, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980136C, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980176C, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980236C, 0x00008040);
    mvPpWriteReg(MV_PP_DEV0, 0x0980276C, 0x00008040);
    mvOsDelay(1);
    /* Un reset dtl_rx */
    mvPpWriteReg(MV_PP_DEV0, 0x0980036C, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980076C, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980136C, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980176C, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980236C, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x0980276C, 0x00008000);
    mvOsDelay(10);
/*    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000004);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000005);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000006);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000007);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000009);*/
    mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00000000);
    mvOsDelay(1);
    /* CAL Start */
    mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00008000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00000000);
    /* Wait for RxClk_x2 */
    mvOsDelay(10);
/*    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000004);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000005);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000006);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000007);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000009);*/
    /* Set RxInit to 0x1 (remember that bit 3 is already set to 0x1) */
    mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000018);
    mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000018);
    /* Wait for p_clk = 1 and p_clk = 0 */
    mvOsDelay(10);
/*  mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000004);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000005);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000006);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000007);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000009); */
    /* Set RxInit to 0x0 */
    mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    /* Wait for ALL PHY_RDY = 1 (0x09800008 bit 0) */
    mvOsDelay(10);
 /* mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000004);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000005);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000006);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000007);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000009);*/
    mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000028);
    mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000028);
    mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000028);
    mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000028);
    mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000028);
    mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000028);
    mvOsDelay(10);
/*    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000000);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000001);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000002);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000003);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000004);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000005);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000006);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000007);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000008);
    mvPpWriteReg(MV_PP_DEV0, 0x000000F0, 0x00000009);*/
    /* Deassert GIG MAC Reset */
    mvPpWriteReg(MV_PP_DEV0, 0x0A806008, 0x0000C008);
    mvPpWriteReg(MV_PP_DEV0, 0x0A806408, 0x0000C008);
    mvPpWriteReg(MV_PP_DEV0, 0x0A806808, 0x0000C008);
    mvPpWriteReg(MV_PP_DEV0, 0x0A806C08, 0x0000C008);
    mvOsDelay(1);
    /* Disable AN */
    mvPpWriteReg(MV_PP_DEV0, 0x0A80600C, 0x00009068);
    mvPpWriteReg(MV_PP_DEV0, 0x0A80640C, 0x00009068);
    mvPpWriteReg(MV_PP_DEV0, 0x0A80680C, 0x00009068);
    mvPpWriteReg(MV_PP_DEV0, 0x0A806C0C, 0x00009068);
    mvPpWriteReg(MV_PP_DEV0, 0x0A800008, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A801008, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A802008, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A803008, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A804008, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A805008, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A800408, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A801408, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A802408, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A803408, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A804408, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A805408, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A800808, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A801808, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A802808, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A803808, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A804808, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A805808, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A800C08, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A801C08, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A802C08, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A803C08, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A804C08, 0x0000C009);
    mvPpWriteReg(MV_PP_DEV0, 0x0A805C08, 0x0000C009);
    mvOsDelay(1);

    /* PHY config started here: */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C400FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0144000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C400FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01641CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C400FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02242299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C400FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01641D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C400FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03042800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C400FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02E42001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A40010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C44018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A40003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C40002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00049140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C500FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0145000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C500FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01651CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C500FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02252299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C500FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01651D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C500FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03052800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C500FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02E52001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A50010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C54018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A50003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C50002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00059140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C600FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0146000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C600FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01661CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C600FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02262299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C600FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01661D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C600FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03062800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C600FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02E62001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A60010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C64018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A60003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C60002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00069140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C700FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0147000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C700FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01671CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C700FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02272299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C700FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01671D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C700FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03072800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C700FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02E72001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A70010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C74018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A70003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C70002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00079140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C800FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0148000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C800FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01681CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C800FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02282299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C800FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01681D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C800FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03082800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C800FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02E82001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A80010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C84018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A80003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C80002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00089140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C900FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0149000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C900FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01691CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C900FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02292299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C900FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01691D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C900FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03092800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C900FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02E92001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A90010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C94018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A90003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03C90002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03A90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00099140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x014A000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016A1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022A2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016A1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x030A2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02EA2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AA0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CA4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AA0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CA0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x000A9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x014B000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016B1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022B2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016B1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x030B2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02EB2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AB0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CB4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AB0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CB0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x000B9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x014C000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016C1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022C2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016C1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x030C2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02EC2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AC0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CC4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AC0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CC0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x000C9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x014D000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016D1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022D2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016D1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x030D2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ED2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AD0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CD4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AD0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CD0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x000D9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x014E000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016E1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022E2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016E1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x030E2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02EE2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AE0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CE4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AE0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CE0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x000E9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x014F000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016F1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022F2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x016F1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x030F2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02EF2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AF0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CF4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AF0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03CF0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03AF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x000F9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C400FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x0144000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C400FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01641CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C400FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02242299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C400FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01641D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C400FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03042800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C400FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02E42001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A40010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C44018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A40003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C40002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C40000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00049140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C500FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x0145000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C500FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01651CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C500FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02252299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C500FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01651D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C500FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03052800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C500FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02E52001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A50010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C54018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A50003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C50002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C50000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00059140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C600FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x0146000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C600FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01661CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C600FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02262299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C600FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01661D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C600FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03062800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C600FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02E62001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A60010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C64018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A60003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C60002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C60000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00069140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C700FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x0147000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C700FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01671CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C700FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02272299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C700FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01671D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C700FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03072800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C700FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02E72001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A70010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C74018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A70003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C70002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C70000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00079140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C800FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x0148000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C800FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01681CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C800FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02282299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C800FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01681D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C800FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03082800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C800FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02E82001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A80010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C84018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A80003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C80002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C80000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00089140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C900FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x0149000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C900FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01691CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C900FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02292299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C900FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x01691D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C900FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03092800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C900FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02E92001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A90010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C94018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A90003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03C90002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03A90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02C90000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x00099140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x014A000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016A1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022A2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016A1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x030A2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02EA2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AA0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CA4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AA0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CA0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CA0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000A9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x014B000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016B1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022B2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016B1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x030B2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02EB2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AB0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CB4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AB0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CB0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CB0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000B9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x014C000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016C1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022C2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016C1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x030C2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02EC2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AC0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CC4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AC0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CC0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CC0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000C9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x014D000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016D1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022D2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016D1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x030D2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ED2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AD0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CD4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AD0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CD0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CD0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000D9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x014E000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016E1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022E2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016E1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x030E2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02EE2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AE0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CE4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AE0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CE0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CE0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000E9140);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x014F000F);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016F1CF0);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022F2299);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF00FD);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x016F1D70);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x030F2800);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF00FF);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02EF2001);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AF0010);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CF4018);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AF0003);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03CF0002);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03AF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02CF0000);
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x000F9140);
    /*  Port[11..0], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004100, 0x337BFF00);
    /*  Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004100, 0x337BFF00);
    /*  Port[11..0], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04004104, 0x404014B4);
    /*  Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05004104, 0x404014B4);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04804104, 0x55552222);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05804104, 0x55552222);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04804108, 0x00000000);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05804108, 0x00000000);
    /*  Act_ & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0400410C, 0x0000E000);
    /*  Act_ & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0500410C, 0x0000E000);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0480410C, 0x00000000);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0580410C, 0x00000000);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04804100, 0x00011000);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05804100, 0x00011000);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x05005110, 0x00000010);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x04005110, 0x00000010);
    /* RefClk will be PECL level; COMMON_0_PU=1; re-writing this reg. */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0000009C, 0x061B0EE3);
    /* SyncE clock on RCVR_CLK_OUT */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x000000E8, 0x00004000);
    /* SP24 recovered clock 0/1 masking enable and div_by_5 */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0980300C, 0x00000099);
    /* SP24 divider reset de-assert */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x09803010, 0x00000020);
    /* SP25 recovered clock 0/1 masking enable and div_by_5 */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0980340C, 0x00000099);
    /* SP25 divider reset de-assert */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x09803410, 0x00000020);
    /* SP26 recovered clock 0/1 masking enable and div_by_5 */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0980400C, 0x00000099);
    /* SP26 divider reset de-assert */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x09804010, 0x00000020);
    /* SP27 recovered clock 0/1 masking enable and div_by_5 */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x0980440C, 0x00000099);
    /* SP27 divider reset de-assert */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x09804410, 0x00000020);
    /* DevEn */
    mvOsDelay(1);mvPpWriteReg(MV_PP_DEV0, 0x00000058, 0x04184003);

    return MV_OK;
}

/*******************************************************************************
* simulate_PP_EEPROM_DB_xCat2A0_QFP_24GE_2SFP
*
* DESCRIPTION:
*       Simulates PP-EEPROM for RD-xCat2-QFP-24GE board to enable
*       network ports.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE    - success
*       MV_FALSE   - failure
*
* COMMENTS:
*       Register writes of PP are now logged in the write log facility.
*
*******************************************************************************/
MV_STATUS simulate_PP_EEPROM_DB_xCat2A0_QFP_24GE_2SFP(void)
{
    /**************************************************************************/
    /* Created By: manually Moti Zehavi for ESQL */
    /* System : DB-xCat2-24GE-2SFP (2L board) */
    /* Core clock : 167MHz */
    /* Serdes refrence clock : 125MHz Internal */
    /* NP Ports mode:QSGMII */
    /* SP24 : NA */
    /* SP25 : NA */
    /* SP26 Mode:SGMII Baud rate:3.125G Reach ability:LR */
    /* SP27 Mode:SGMII Baud rate:3.125G Reach ability:LR */
    /**************************************************************************/
    /* rev 3.01 	08-08-2010		 */
    /* MACSec dis, turn off analog pll on ports 24,25 (NC), 26 MAC LoopBack */
    /**************************************************************************/
    /* set SMI0 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004200, 0x00070000);
    /* set SMI1 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004200, 0x00070000);
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000005);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000006);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000007);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000009);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000000a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000000b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000000c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000000d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000000e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000000f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000011);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000013);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000014);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000015);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000016);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000017);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000019);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000001a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000001b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000001c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000001d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000001e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x0000001f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000020);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000021);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000022);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000023);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000024);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000025);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000026);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000027);

    /******************************************************************** */
    /* ********************** xCat2 section ******************* */
    /******************************************************************** */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x308F8004, 0x00000001);      /*Pipe0 access */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x30804050, 0x00020003);      /*PCL TCAM */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x3080C050, 0x00020003);      /*RTR TCAM */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x008F8304, 0x00000048);      /*changed in xCat2, GPP PAD control */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x008F8314, 0x00000048);      /*changed in xCat2, SSMII PAD control */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x008F8300, 0x00000048);      /*changed in xCat2, LED PAD control */
    /* User defined register */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f4, 0x01010111);
    /* ANA_GRP configuration (SE or differential select) */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000009c, 0x061b0cc3);      /*disable PECL receiver and common_0_PECL_EN=CMOS */
    /* ------------- Configuring network SD to 5Gbps (QSGMII) */
    /* Pre Power-Up Serdes No.0  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);    /* SDResetIn=1 */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.0  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980020c, 0x0000800a);    /* ref freq select = 62.5MHz */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800210, 0x00004414);    /* interpolator current = 25uA, rx_speed_div_force=1, tx_speed_div_force=1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800214, 0x0000a150);    /*fbdiv=0x50,refdiv=by 4 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800218, 0x0000baab);    /*sel_v2i=0x3, hi_v2i=0x2, loadcap_pll_force=1, sel_lpfres_force=1, speed_pll=0x3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980021c, 0x0000882c);    /*rxdcccal_done_c=1, rxdcccal_done_e=1, icp=0x8 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098003cc, 0x00002000);    /*Analog reserved reg 0, bit 11=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980022c, 0x00000000);    /*EXT_RXIMP=0, EXT_TXIMP=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800230, 0x00000000);    /*pwrseq_cal_max_speed_en=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800234, 0x00004000);    /*rx_imp_vthimpcal=0, sampler_os_scale=1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980023c, 0x00000018);    /*process_rd=0x8 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800250, 0x0000a0C0);    /*TX_DRV_CM=0xC, DCC_EN=1 */
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800250, 0x0000a060);    /*TX_DRV_CM=0x6 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800254, 0x0000750c);    /*Tx_Emph_Amp=0xC, TX_AMP_ADJ=0x3, different from LionB */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980027c, 0x000090aa);    /*SEL_MUFF=0x2, SEL_MUFI=0x2,rxckdig_speed=0x1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800280, 0x00000800);    /*DTL_FLOOP_EN=0x0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980028c, 0x00000363);    /*ffe_cap_sel=0x3, ffe_res_sel=0x6, ffe_isel=0x3, different from LionB */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980035c, 0x0000423f);    /*slc_en=0x3f, tx_imp_r50=0x1(43Ohm) */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00000000);    /*ana_clk_det_en=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800234, 0x00004000);    /*rx_imp_vthimpcal=0, sampler_os_scale=1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800228, 0x0000e0c0);    /*IMP_VTHIMPCAL=0x4 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00008040);    /*ana_clk_det_en=1, reset_dtl_rx */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00008000);    /*de-assert reset_dtl_rx */
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00000000);
    /* Post Power-Up Serdes No.0  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000018);    /*SDRxInit=1 */
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);    /*SDRxInit=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800000, 0x00003e80);    /*default value */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000018);    /*SDRxInit=1 */
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);    /*SDRxInit=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000028);    /*RFResetIn=1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800000, 0x00003e80);    /*default value */
    /* Pre Power-Up Serdes No.1  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400, 0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.1  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980060c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800610, 0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800614, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800618, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980061c, 0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098007cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980062c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800630, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980063c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800650, 0x0000a0C0);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800650, 0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800654, 0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980067c, 0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800680, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980068c, 0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980075c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800628, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00000000);
    /* Post Power-Up Serdes No.1  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400, 0x00003e80);
    /* Pre Power-Up Serdes No.2  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000, 0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.2  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980120c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801210, 0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801214, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801218, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980121c, 0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098013cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980122c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801230, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980123c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801250, 0x0000a0C0);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801250, 0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801254, 0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980127c, 0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801280, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980128c, 0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980135c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801228, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00000000);
    /* Post Power-Up Serdes No.2  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000, 0x00003e80);
    /* Pre Power-Up Serdes No.3  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801400, 0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.3  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980160c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801610, 0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801614, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801618, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980161c, 0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098017cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980162c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801630, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980163c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801650, 0x0000a0C0);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801650, 0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801654, 0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980167c, 0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801680, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980168c, 0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980175c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801628, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00000000);
    /* Post Power-Up Serdes No.3  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801400, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801400, 0x00003e80);
    /* Pre Power-Up Serdes No.4  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000, 0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.4  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980220c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802210, 0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802214, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802218, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980221c, 0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098023cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980222c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802230, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980223c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802250, 0x0000a0C0);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802250, 0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802254, 0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980227c, 0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802280, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980228c, 0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980235c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980236c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802228, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980236c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980236c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802224, 0x00000000);
    /* Post Power-Up Serdes No.4  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000, 0x00003e80);
    /* Pre Power-Up Serdes No.5  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400, 0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.5  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980260c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802610, 0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802614, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802618, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980261c, 0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098027cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980262c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802630, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980263c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802650, 0x0000a0C0);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802650, 0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802654, 0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980267c, 0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802680, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980268c, 0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980275c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980276c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802628, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980276c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980276c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802624, 0x00000000);
    /* Post Power-Up Serdes No.5  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400, 0x00003e80);
    /* NP ports de-assert MAC reset  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800008, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800408, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800808, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800c08, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801008, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801408, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801808, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801c08, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802008, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802408, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802808, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802c08, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803008, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803408, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803808, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803c08, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804008, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804408, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804808, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804c08, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805008, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805408, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805808, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805c08, 0x0000c008);
    /* --------------------- SP configuration */

    /* SP26 ---------------- Serdes configuration -----------------  */
    /* Pre Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Internal registers of lane No.0 to 1.25Gbps */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980420c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804210, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804214, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804218, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980421c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098043cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980422c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804230, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980423c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804250, 0x000080C0);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804250, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804254, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980427c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804280, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980428c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980435c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980436c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804228, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980436c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980436c, 0x00008000);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804224, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804224, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804224, 0x00000000);
    /* Post Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000, 0x0000be80);
    /* SP27 ---------------- Serdes configuration -----------------  */
    /* Pre Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Internal registers of lane No.0 to 1.25Gbps */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980460c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804610, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804614, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804618, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980461c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098047cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980462c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804630, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980463c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804650, 0x000080C0);
    /* Delay 5mS ----------------  */
    mvOsDelay(5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804650, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804654, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980467c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804680, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980468c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980475c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804628, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c, 0x00008000);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624, 0x00000000);
    /* Post Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400, 0x0000be80);

    /* De-assert Gig MAC Reset Stacking port 26  --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806808, 0x0000c008);
    /* Port26 in-band auto-negotiation disable --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80680c, 0x00009068);
    /* De-assert Gig MAC Reset Stacking port 27  --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c08, 0x0000c008);
    /* Port27 in-band auto-negotiation disable --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c0c, 0x00009068);

    /* ----------- Led stream configuration ----------- */
    /* Port[11..0], Led stream ordered by port & Link effect 'on' */
    /* Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004100, 0x337bff00);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004100, 0x337bff00);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004104, 0x404014b4);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004104, 0x404014b4);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04804104, 0x55552222);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05804104, 0x55552222);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04804108, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05804108, 0x00000000);
    /* ~Act & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0400410c, 0x0000e000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0500410c, 0x0000e000);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0480410c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0580410c, 0x00000000);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04804100, 0x00011000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05804100, 0x00011000);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05005110, 0x00000010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04005110, 0x00000010);
    /* Power saving on xCat2 - turn off analog PLL, Rx, Tx on ports 24,25 (NC)  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000040);

    /* ********************** 88E1545 PHYs Erratas & Settings ******************* */
    /* ------------- 88E1545 QSGMII Phy configuration on LMS0 ------------- */
    /* Register structure */
    /* bits: |  25-21  |  20-16  | 15-0 | */
    /* data: | Reg add | PHY add | Data | */

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c40003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02041177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02248811);
    /* disable MACSec */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c40012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03640000);
    /* end of disable MACSec */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c40000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c50003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02051177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02258811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c50012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03650000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c50000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c60003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02061177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02268811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c60012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03660000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c60000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c70003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02071177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02278811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c70012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03670000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c70000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c80003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02081177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02288811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c80012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03680000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c80000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c90003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02091177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02298811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c90012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x03690000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c90000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ca0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x020a1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022a8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ca0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x036a0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ca0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cb0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x020b1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022b8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cb0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x036b0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cb0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cc0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x020c1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022c8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cc0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x036c0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cc0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cd0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x020d1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022d8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cd0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x036d0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cd0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ce0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x020e1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022e8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ce0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x036e0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02ce0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cf0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x020f1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022f8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cf0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x036f0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02cf0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c40003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02041177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02248811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c40012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03640000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c40000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c50003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02051177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02258811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c50012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03650000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c50000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c60003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02061177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02268811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c60012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03660000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c60000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c70003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02071177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02278811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c70012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03670000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c70000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c80003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02081177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02288811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c80012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03680000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c80000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c90003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02091177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02298811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c90012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x03690000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02c90000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ca0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x020a1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022a8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ca0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x036a0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ca0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cb0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x020b1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022b8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cb0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x036b0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cb0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cc0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x020c1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022c8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cc0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x036c0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cc0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cd0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x020d1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022d8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cd0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x036d0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cd0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ce0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x020e1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022e8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ce0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x036e0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02ce0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cf0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x020f1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x022f8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cf0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x036f0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054, 0x02cf0000);

    /* ---------- Device enable ----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000058, 0x04184003);

    return MV_OK;
}

/*******************************************************************************
 * simulate_PP_EEPROM_DB_xCat2A1_QFP_24GE_2SFP
 */
MV_STATUS simulate_PP_EEPROM_DB_xCat2A1_QFP_24GE_2SFP(void)
{
    /*///////////////////////////////////////////////////////////////////// */
    /* Created By: manually Moti Zehavi for ESQL */
    /* System : DB-xCat2-24GE-2SFP (2L board) */
    /* Core clock : 167MHz */
    /* Serdes refrence clock : 125MHz Internal */
    /* NP Ports mode:QSGMII */
    /* SP24 : NA */
    /* SP25 : NA */
    /* SP26 Mode:1000Base-X */
    /* SP27 Mode:1000Base-X */
    /*///////////////////////////////////////////////////////////////////// */
    /* rev 3.1 	18-08-2010		 */
    /* MACSec dis, turn off analog pll on ports 24,25 (NC) */
    /*///////////////////////////////////////////////////////////////////// */
    /* set SMI0 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004200 ,0x00070000);
    /* set SMI1 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004200 ,0x00070000);

    /******************************************************************** */
    /* ********************** xCat2 section ******************* */
    /******************************************************************** */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x308F8004 ,0x00000001);      /*Pipe0 access */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x30804050 ,0x00020003);      /*PCL TCAM */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x3080C050 ,0x00020003);      /*RTR TCAM */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x008F8304 ,0x00000048);      /*changed in xCat2, GPP PAD control */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x008F8314 ,0x00000048);      /*changed in xCat2, SSMII PAD control */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x008F8310 ,0x00000048); */ /*changed in xCat2, RGMII PAD control */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x008F8300 ,0x00000048);      /*changed in xCat2, LED PAD control */
    /* User defined register */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f4 ,0x01010111);
    /* ANA_GRP configuration (SE or differential select) */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000009c ,0x061b0cc3);      /*disable PECL receiver and common_0_PECL_EN=CMOS */
    /* ------------- Configuring network SD to 5Gbps (QSGMII) */
    /* Pre Power-Up Serdes No.0  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800000 ,0x00003e80);*//* default value */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004 ,0x00000008);    /* SDResetIn=1 */
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    /* Configuring Network Serdes Internal registers to desired frequency.0  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980020c ,0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800210 ,0x00004414);    /* interpolator current = 25uA, rx_speed_div_force=1, tx_speed_div_force=1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800214 ,0x0000a150);    /*fbdiv=0x50,refdiv=by 4 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800218 ,0x0000baab);    /*sel_v2i=0x3, hi_v2i=0x2, loadcap_pll_force=1, sel_lpfres_force=1, speed_pll=0x3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980021c ,0x0000882c);    /*rxdcccal_done_c=1, rxdcccal_done_e=1, icp=0x8 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098003cc ,0x00002000);    /*Analog reserved reg 0, bit 11=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980022c ,0x00000000);    /*EXT_RXIMP=0, EXT_TXIMP=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800230 ,0x00000000);    /*pwrseq_cal_max_speed_en=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800234 ,0x00004000);    /*rx_imp_vthimpcal=0, sampler_os_scale=1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980023c ,0x00000018);    /*process_rd=0x8 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800250 ,0x0000a0C0);    /*TX_DRV_CM=0xC, DCC_EN=1 */
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800250 ,0x0000a060);    /*TX_DRV_CM=0x6 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800254 ,0x0000750c);    /*Tx_Emph_Amp=0xC, TX_AMP_ADJ=0x3 */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800258 ,0x00000000);*//*default value, different from LionB */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980027c ,0x000090aa);    /*SEL_MUFF=0x2, SEL_MUFI=0x2,rxckdig_speed=0x1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800280 ,0x00000800);    /*DTL_FLOOP_EN=0x0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980028c ,0x00000363);    /*ffe_cap_sel=0x3, ffe_res_sel=0x6, ffe_isel=0x3 */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800298 ,0x0000000a);*//* default value */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980035c ,0x0000423f);    /*slc_en=0x3f, tx_imp_r50=0x1(43Ohm) */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800364 ,0x00005555);*//* default value */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c ,0x00000000);    /*ana_clk_det_en=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800234 ,0x00004000);    /*rx_imp_vthimpcal=0, sampler_os_scale=1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800228 ,0x0000e0c0);    /*IMP_VTHIMPCAL=0x4 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c ,0x00008040);    /*ana_clk_det_en=1, reset_dtl_rx */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c ,0x00008000);    /*de-assert reset_dtl_rx */
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(20);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224 ,0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224 ,0x00000000);
    /* Post Power-Up Serdes No.0  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004 ,0x00000018);    /*SDRxInit=1 */
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004 ,0x00000008);    /*SDRxInit=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800000 ,0x00003e80);    /*default value */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004 ,0x00000018);    /*SDRxInit=1 */
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004 ,0x00000008);    /*SDRxInit=0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004 ,0x00000028);    /*RFResetIn=1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800000 ,0x00003e80);    /*default value */
    /* Pre Power-Up Serdes No.1  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400 ,0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404 ,0x00000008);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    /* Configuring Network Serdes Internal registers to desired frequency.1  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980060c ,0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800610 ,0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800614 ,0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800618 ,0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980061c ,0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098007cc ,0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980062c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800630 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800634 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980063c ,0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800650 ,0x0000a0C0);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800650 ,0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800654 ,0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980067c ,0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800680 ,0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980068c ,0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980075c ,0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800634 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800628 ,0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c ,0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c ,0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(20);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624 ,0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624 ,0x00000000);
    /* Post Power-Up Serdes No.1  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400 ,0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404 ,0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400 ,0x00003e80);
    /* Pre Power-Up Serdes No.2  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000 ,0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004 ,0x00000008);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    /* Configuring Network Serdes Internal registers to desired frequency. */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980120c ,0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801210 ,0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801214 ,0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801218 ,0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980121c ,0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098013cc ,0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980122c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801230 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801234 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980123c ,0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801250 ,0x0000a0C0);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801250 ,0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801254 ,0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980127c ,0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801280 ,0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980128c ,0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980135c ,0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801234 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801228 ,0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c ,0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c ,0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(20);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224 ,0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224 ,0x00000000);
    /* Post Power-Up Serdes No.2  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000 ,0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004 ,0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000 ,0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404 ,0x00000008);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    /* Configuring Network Serdes Internal registers to desired frequency.3  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980160c ,0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801610 ,0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801614 ,0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801618 ,0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980161c ,0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098017cc ,0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980162c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801630 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801634 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980163c ,0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801650 ,0x0000a0C0);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801650 ,0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801654 ,0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980167c ,0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801680 ,0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980168c ,0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980175c ,0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801634 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801628 ,0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c ,0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c ,0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(20);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624 ,0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624 ,0x00000000);
    /* Post Power-Up Serdes No.3  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801400 ,0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404 ,0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801400 ,0x00003e80);
    /* Pre Power-Up Serdes No.4  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000 ,0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004 ,0x00000008);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    /* Configuring Network Serdes Internal registers to desired frequency.4  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980220c ,0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802210 ,0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802214 ,0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802218 ,0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980221c ,0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098023cc ,0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980222c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802230 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802234 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980223c ,0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802250 ,0x0000a0C0);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802250 ,0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802254 ,0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980227c ,0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802280 ,0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980228c ,0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980235c ,0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980236c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802234 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802228 ,0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980236c ,0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980236c ,0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(20);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802224 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802224 ,0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802224 ,0x00000000);
    /* Post Power-Up Serdes No.4  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000 ,0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004 ,0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000 ,0x00003e80);
    /* Pre Power-Up Serdes No.5  */
    /*mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400 ,0x00003e80); */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404 ,0x00000008);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    /* Configuring Network Serdes Internal registers to desired frequency. */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980260c ,0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802610 ,0x00004414);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802614 ,0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802618 ,0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980261c ,0x0000882c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098027cc ,0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980262c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802630 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802634 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980263c ,0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802650 ,0x0000a0C0);
    /* -------Delay 5mS ----------------  */
    mvOsDelay(10);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802650 ,0x0000a060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802654 ,0x0000750c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980267c ,0x000090aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802680 ,0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980268c ,0x00000363);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980275c ,0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980276c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802634 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802628 ,0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980276c ,0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980276c ,0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(20);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802624 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802624 ,0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802624 ,0x00000000);
    /* Post Power-Up Serdes No.5  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400 ,0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404 ,0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400 ,0x00003e80);
    /* NP ports de-assert MAC reset  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800008 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800408 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800808 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800c08 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801008 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801408 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801808 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801c08 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802008 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802408 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802808 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802c08 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803008 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803408 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803808 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803c08 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804008 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804408 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804808 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804c08 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805008 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805408 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805808 ,0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805c08 ,0x0000c008);
    /* --------------------- SP configuration */

    /* SP26 ---------------- Serdes configuration -----------------  */
    /* Pre Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000 ,0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004 ,0x00000008);
    /* ---------Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000004);
    /* Configuring Internal registers of lane No.0 to 1.25Gbps */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980420c ,0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804210 ,0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804214 ,0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804218 ,0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980421c ,0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098043cc ,0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980422c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804230 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804234 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980423c ,0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804250 ,0x000080C0);
    /* ---------Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804250 ,0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804254 ,0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980427c ,0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804280 ,0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980428c ,0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980435c ,0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980436c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804234 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804228 ,0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980436c ,0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980436c ,0x00008000);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804224 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804224 ,0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804224 ,0x00000000);
    /* Post Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000 ,0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004 ,0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000 ,0x0000be80);
    /* SP27 ---------------- Serdes configuration -----------------  */
    /* Pre Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400 ,0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404 ,0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000004);
    /* Configuring Internal registers of lane No.0 to 1.25Gbps */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980460c ,0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804610 ,0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804614 ,0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804618 ,0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980461c ,0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098047cc ,0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980462c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804630 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804634 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980463c ,0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804650 ,0x000080C0);
    /* ---------Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0 ,0x00000004);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804650 ,0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804654 ,0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980467c ,0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804680 ,0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980468c ,0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980475c ,0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804634 ,0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804628 ,0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c ,0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c ,0x00008000);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624 ,0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624 ,0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624 ,0x00000000);
    /* Post Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400 ,0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404 ,0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404 ,0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404 ,0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400 ,0x0000be80);

    /* De-assert Gig MAC Reset Stacking port 26  ------------ */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806808 ,0x0000c008);
    /* Port26 in-band auto-negotiation disable --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80680c ,0x00009068);
    /* De-assert Gig MAC Reset Stacking port 27  ------------ */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c08 ,0x0000c008);
    /* Port27 in-band auto-negotiation disable --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c0c ,0x00009068);

    /* ***************** 88E1545 PHYs Settings ****************** */
    /* ------------- 88E1545 QSGMII Phy configuration on LMS0 ------------- */
    /* Register structure */
    /* bits: |  25-21  |  20-16  | 15-0 | */
    /* data: | Reg add | PHY add | Data | */
    /* */
    /* Setting 1545 PHY LEDs and MACSec dissable */
    /* */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c40003);	/* Set page for LEDs settings */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02041177);	/* Set LEDs */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02248811);	/* Set LEDs */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c40012);	/* Set page to disable MACSec */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x03640000);	/* Disable MACSec */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c40000);	/* Return page to 0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c50003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02051177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02258811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c50012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x03650000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c50000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c60003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02061177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02268811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c60012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x03660000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c60000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c70003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02071177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02278811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c70012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x03670000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c70000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c80003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02081177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02288811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c80012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x03680000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c80000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c90003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02091177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02298811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c90012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x03690000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02c90000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02ca0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x020a1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x022a8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02ca0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x036a0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02ca0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cb0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x020b1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x022b8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cb0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x036b0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cb0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cc0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x020c1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x022c8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cc0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x036c0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cc0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cd0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x020d1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x022d8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cd0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x036d0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cd0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02ce0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x020e1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x022e8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02ce0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x036e0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02ce0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cf0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x020f1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x022f8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cf0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x036f0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054 ,0x02cf0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c40003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02041177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02248811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c40012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x03640000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c40000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c50003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02051177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02258811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c50012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x03650000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c50000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c60003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02061177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02268811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c60012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x03660000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c60000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c70003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02071177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02278811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c70012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x03670000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c70000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c80003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02081177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02288811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c80012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x03680000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c80000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c90003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02091177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02298811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c90012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x03690000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02c90000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02ca0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x020a1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x022a8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02ca0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x036a0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02ca0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cb0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x020b1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x022b8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cb0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x036b0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cb0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cc0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x020c1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x022c8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cc0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x036c0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cc0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cd0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x020d1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x022d8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cd0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x036d0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cd0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02ce0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x020e1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x022e8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02ce0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x036e0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02ce0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cf0003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x020f1177);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x022f8811);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cf0012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x036f0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004054 ,0x02cf0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x02c40004);	/* Set page for disable output clock */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x03643E80);	/* Disable output clock of the right phy */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x02c40000);	/* Return page to 0 */

    /* Phy rev A0 Power up sequence */

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C400fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1040010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C400fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x244099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x641120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x164113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1C48100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1E4112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C400fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2420b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C400ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2240000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2244444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2248064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2240108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2240f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2248c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x204214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2240f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x204214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x224ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x204214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22439aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x204214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2248433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2242010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22499eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2242f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x224584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2241223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2042158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C40000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C500fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1050010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C500fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x254099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x651120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x165113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1C58100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1E5112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C500fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2520b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C500ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2250000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2254444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2258064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2250108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2250f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2258c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x205214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2250f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x205214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x225ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x205214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22539aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x205214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2258433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2252010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22599eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2252f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x225584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2251223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2052158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C50000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C600fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1060010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C600fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x264099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x661120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x166113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1C68100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1E6112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C600fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2620b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C600ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2260000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2264444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2268064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2260108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2260f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2268c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x206214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2260f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x206214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x226ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x206214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22639aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x206214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2268433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2262010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22699eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2262f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x226584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2261223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2062158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C60000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C700fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1070010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C700fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x274099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x671120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x167113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1C78100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1E7112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C700fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2720b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C700ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2270000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2274444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2278064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2270108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2270f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2278c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x207214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2270f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x207214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x227ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x207214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22739aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x207214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2278433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2272010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22799eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2272f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x227584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2271223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2072158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C70000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C800fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1080010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C800fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x284099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x681120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x168113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1C88100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1E8112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C800fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2820b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C800ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2280000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2284444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2288064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2280108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2280f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2288c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x208214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2280f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x208214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x228ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x208214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22839aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x208214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2288433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2282010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22899eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2282f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x228584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2281223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2082158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C80000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C900fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1090010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C900fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x294099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x691120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x169113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1C98100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1E9112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C900fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2920b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C900ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2290000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2294444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2298064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2290108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2290f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2298c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x209214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2290f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x209214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x229ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x209214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22939aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x209214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2298433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2292010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22999eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2292f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x229584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2291223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2092158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2C90000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ca00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x10a0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ca00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2a4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x6a1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x16a113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Ca8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Ea112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ca00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2a20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ca00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22aba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22a1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20a2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ca0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cb00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x10b0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cb00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2b4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x6b1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x16b113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Cb8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Eb112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cb00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2b20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cb00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22bba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22b1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20b2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cb0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cc00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x10c0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cc00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2c4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x6c1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x16c113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Cc8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Ec112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cc00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2c20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cc00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22cba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22c1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20c2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cc0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cd00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x10d0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cd00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2d4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x6d1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x16d113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Cd8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Ed112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cd00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2d20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cd00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22dba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22d1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20d2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cd0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ce00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x10e0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ce00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2e4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x6e1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x16e113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Ce8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Ee112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ce00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2e20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ce00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22eba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22e1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20e2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Ce0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cf00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x10f0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cf00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2f4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x6f1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x16f113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Cf8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x1Ef112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cf00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2f20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cf00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22fba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x22f1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x20f2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x84004054 ,0x2Cf0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C400fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1040010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C400fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x244099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x641120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x164113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1C48100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1E4112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C400fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2420b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C400ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2240000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2244444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2248064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2240108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2240f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2248c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x204214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2240f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x204214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x224ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x204214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22439aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x204214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2248433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2242010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22499eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2242f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x224584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2241223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2042158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C40000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C500fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1050010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C500fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x254099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x651120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x165113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1C58100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1E5112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C500fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2520b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C500ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2250000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2254444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2258064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2250108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2250f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2258c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x205214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2250f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x205214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x225ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x205214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22539aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x205214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2258433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2252010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22599eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2252f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x225584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2251223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2052158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C50000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C600fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1060010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C600fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x264099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x661120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x166113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1C68100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1E6112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C600fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2620b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C600ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2260000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2264444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2268064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2260108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2260f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2268c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x206214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2260f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x206214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x226ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x206214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22639aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x206214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2268433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2262010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22699eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2262f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x226584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2261223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2062158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C60000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C700fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1070010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C700fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x274099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x671120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x167113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1C78100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1E7112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C700fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2720b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C700ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2270000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2274444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2278064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2270108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2270f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2278c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x207214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2270f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x207214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x227ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x207214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22739aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x207214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2278433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2272010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22799eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2272f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x227584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2271223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2072158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C70000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C800fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1080010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C800fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x284099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x681120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x168113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1C88100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1E8112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C800fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2820b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C800ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2280000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2284444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2288064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2280108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2280f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2288c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x208214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2280f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x208214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x228ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x208214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22839aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x208214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2288433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2282010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22899eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2282f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x228584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2281223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2082158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C80000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C900fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1090010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C900fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x294099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x691120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x169113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1C98100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1E9112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C900fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2920b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C900ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2290000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2294444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2298064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2290108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2290f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2298c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x209214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2290f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x209214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x229ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x209214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22939aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x209214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2298433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2292010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22999eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2292f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x229584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2291223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2092158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2C90000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ca00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x10a0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ca00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2a4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x6a1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x16a113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Ca8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Ea112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ca00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2a20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ca00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22aba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22a1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20a2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ca0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cb00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x10b0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cb00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2b4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x6b1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x16b113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Cb8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Eb112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cb00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2b20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cb00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22bba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22b1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20b2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cb0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cc00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x10c0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cc00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2c4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x6c1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x16c113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Cc8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Ec112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cc00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2c20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cc00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22cba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22c1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20c2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cc0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cd00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x10d0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cd00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2d4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x6d1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x16d113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Cd8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Ed112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cd00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2d20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cd00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22dba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22d1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20d2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cd0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ce00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x10e0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ce00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2e4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x6e1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x16e113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Ce8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Ee112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ce00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2e20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ce00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22eba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22e1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20e2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Ce0000);

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cf00fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x10f0010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cf00fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2f4099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x6f1120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x16f113c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Cf8100);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x1Ef112a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cf00fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2f20b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cf00ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f0000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f4444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f8064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f0108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f0f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f8c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f0f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22fba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f39aa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f8433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f2010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f99eb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f2f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x22f1223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x20f2158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x85004054 ,0x2Cf0000);

    /* Direct mode for stacking ports */

    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04005114 ,0x1);		/* Set to Direct Mode */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05005114 ,0x1);		/* Set to Direct Mode */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04805104 ,0xABA9ABA9);	/* group function per stack port 10&&11||10&&9 (write to 2 SP) */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05805104 ,0xABA9ABA9);	/* group function per stack port 10&&11||10&&9 (write to 2 SP) */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04005110 ,0x312);		/* Class 9 - Rx, Class 10 - Link, Class 11 - Tx */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05005110 ,0x312);		/* Class 9 - Rx, Class 10 - Link, Class 11 - Tx */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04005104 ,0xA0000000);	/* Set Blink in class 9 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05005104 ,0xA0000000);	/* Set Blink in class 9 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04805100 ,0xA0000000);	/* Set Blink in class 11 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05805100 ,0xA0000000);	/* Set Blink in class 11 */

    /* ---------- Device enable ----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000058 ,0x04184003);

    return MV_OK;
}

/*******************************************************************************
 * mvPpEeprom_sleed_board
 */
MV_STATUS mvPpEeprom_sleed_board(void)
{
    /*/////////////////////////////////////////////////////////////////////// */
    /* Created By: xCat-A1 configurator Rev2.3 */
    /* System : DB-xCat-24GE-4GP */
    /* Core clock : 222MHz */
    /* Serdes refrence clock : 125MHz Internal */
    /* NP Ports mode:SGMII */
    /* SP24 Mode:SGMII Baud rate:1.25G Reach ability:LR */
    /* SP25 Mode:PD Baud rate:1.25G Reach ability:LR */
    /* SP26 Mode:SGMII Baud rate:1.25G Reach ability:LR */
    /* SP27 Mode:SGMII Baud rate:1.25G Reach ability:LR */
    /*/////////////////////////////////////////////////////////////////////// Remember to change NP params with Doron!!!!  */
    /* set SMI0 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004200, 0x00070000);
    /* set SMI1 fast_mdc to div/64 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004200, 0x00070000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000048, 0x1fafc243);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000044, 0xf090fc7e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000024, 0x7efc243f);
    /* Stack mode configuration register */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f4, 0x11110011);
    /* ANA_GRP configuration (SE or differential select) */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000009c, 0x071b8ce3);
    /* Device configuration register bits 0-22 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7e840000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000009c, 0x061b0cc3);
    /* Selects SGMII in network ports */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000005c, 0x001fe00d);
    /* HGS device configuration SGMII or HGS */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7a840000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x7a840000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x6a840000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000028, 0x4a840000);
    /* extended global configuration2 RCVD clock - disable, 2nd RCVD clock - disable, output clock - disable */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x00f3fc00);
    /* extended global configuration2 XAUI/RXAUI select, SP lane 0 or 3 swap - disable */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x10f3fc00);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x10f3fc00);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x10f3fc00);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0000008c, 0x10f3fc00);
    /* Configuring network SD to 1.25Gbps  */
    /* Pre Power-Up Serdes No.0  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.0  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980020c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800210, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800214, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800218, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980021c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098003cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980022c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800230, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980023c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800250, 0x000080c0);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800250, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800254, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800258, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980027c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800280, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980028c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800298, 0x0000000a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980035c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800364, 0x00005555);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800228, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980036c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800224, 0x00000000);
    /* Post Power-Up Serdes No.0  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800000, 0x0000be80);
    /* Pre Power-Up Serdes No.1  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.1  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980060c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800610, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800614, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800618, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980061c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098007cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980062c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800630, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980063c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800650, 0x000080c0);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800650, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800654, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800658, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980067c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800680, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980068c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800698, 0x0000000a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980075c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800764, 0x00005555);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800628, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980076c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800624, 0x00000000);
    /* Post Power-Up Serdes No.1  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09800400, 0x0000be80);
    /* Pre Power-Up Serdes No.2  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.2  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980120c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801210, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801214, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801218, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980121c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098013cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980122c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801230, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980123c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801250, 0x000080c0);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801250, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801254, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801258, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980127c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801280, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980128c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801298, 0x0000000a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980135c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801364, 0x00005555);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801228, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980136c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801224, 0x00000000);
    /* Post Power-Up Serdes No.2  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801000, 0x0000be80);
    /* Pre Power-Up Serdes No.3  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Network Serdes Internal registers to desired frequency.3  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980160c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801610, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801614, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801618, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980161c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098017cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980162c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801630, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980163c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801650, 0x000080c0);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801650, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801654, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801658, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980167c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801680, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980168c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801698, 0x0000000a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980175c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801764, 0x00005555);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801628, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980176c, 0x00008000);
    /* ----------- Delay 10mS ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801624, 0x00000000);
    /* Post Power-Up Serdes No.3  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09801400, 0x0000be80);
    /* Pre Power-Up Serdes No.4  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down SD.4  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802368, 0x00000000);
    /* Post Power-Up Serdes No.4  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802000, 0x0000be80);
    /* Pre Power-Up Serdes No.5  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down SD.5  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802768, 0x00000000);
    /* Post Power-Up Serdes No.5  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09802400, 0x0000be80);
    /* NP ports de-assert MAC reset  // Change Here NP Params */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800000, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80000c, 0x0000906A);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a800008, 0x0000c009);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801000, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80100c, 0x0000906A);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a801008, 0x0000c009);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802000, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80200c, 0x0000906A);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a802008, 0x0000c009);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803000, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80300c, 0x0000906A);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a803008, 0x0000c009);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804000, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80400c, 0x0000b0ec);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a804008, 0x0000c009);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805000, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80500c, 0x0000b0ec);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a805008, 0x0000c009);
    /* SP24 ---------------- Serdes configuration -----------------  */
    /* Pre Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803368, 0x00000000);
    /* Post Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803000, 0x0000be80);
    /* Pre Power-Up Serdes No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803768, 0x00000000);
    /* Post Power-Up Serdes No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09803400, 0x0000be80);
    /* Pre Power-Up Serdes No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804368, 0x00000000);
    /* Post Power-Up Serdes No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804000, 0x0000be80);
    /* Pre Power-Up Serdes No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Internal registers of lane No.3 to 1.25Gbps */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980460c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804610, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804614, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804618, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980461c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098047cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980462c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804630, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980463c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804650, 0x000080c0);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804650, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804654, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804658, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980467c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804680, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980468c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804698, 0x0000000a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980475c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804764, 0x00005555);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804634, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804628, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980476c, 0x00008000);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804624, 0x00000000);
    /* Post Power-Up Serdes No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09804400, 0x0000be80);
    /* SP25 ---------------- Serdes configuration -----------------  */
    /* Pre Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805000, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805368, 0x00000000);
    /* Post Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805000, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805000, 0x00003e80);
    /* Pre Power-Up Serdes No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805400, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805768, 0x00000000);
    /* Post Power-Up Serdes No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805400, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09805400, 0x00003e80);
    /* Pre Power-Up Serdes No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806000, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806368, 0x00000000);
    /* Post Power-Up Serdes No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806000, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806000, 0x00003e80);
    /* Pre Power-Up Serdes No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806400, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806768, 0x00000000);
    /* Post Power-Up Serdes No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806400, 0x00003e80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09806400, 0x00003e80);
    /* SP26 ---------------- Serdes configuration -----------------  */
    /* Pre Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Internal registers of lane No.0 to 1.25Gbps */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980720c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807210, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807214, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807218, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980721c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098073cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980722c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807230, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980723c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807250, 0x000080c0);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807250, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807254, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807258, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980727c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807280, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980728c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807298, 0x0000000a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980735c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807364, 0x00005555);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980736c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807228, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980736c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980736c, 0x00008000);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807224, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807224, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807224, 0x00000000);
    /* Post Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807000, 0x0000be80);
    /* Pre Power-Up Serdes No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807768, 0x00000000);
    /* Post Power-Up Serdes No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09807400, 0x0000be80);
    /* Pre Power-Up Serdes No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808368, 0x00000000);
    /* Post Power-Up Serdes No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808000, 0x0000be80);
    /* Pre Power-Up Serdes No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808768, 0x00000000);
    /* Post Power-Up Serdes No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09808400, 0x0000be80);
    /* SP27 ---------------- Serdes configuration -----------------  */
    /* Pre Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Configuring Internal registers of lane No.0 to 1.25Gbps */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980920c, 0x0000800a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809210, 0x00006614);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809214, 0x0000a150);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809218, 0x0000baab);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980921c, 0x00008b2c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x098093cc, 0x00002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980922c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809230, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980923c, 0x00000018);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809250, 0x000080c0);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809250, 0x00008060);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809254, 0x0000770a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809258, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980927c, 0x0000905a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809280, 0x00000800);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980928c, 0x00000266);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809298, 0x0000000a);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980935c, 0x0000423f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809364, 0x00005555);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980936c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809234, 0x00004000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809228, 0x0000e0c0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980936c, 0x00008040);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980936c, 0x00008000);
    /* ----------- Delay 10mS  ----------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809224, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809224, 0x00008000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809224, 0x00000000);
    /* Post Power-Up Serdes No.0 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809000, 0x0000be80);
    /* Pre Power-Up Serdes No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809768, 0x00000000);
    /* Post Power-Up Serdes No.1 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x09809400, 0x0000be80);
    /* Pre Power-Up Serdes No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a368, 0x00000000);
    /* Post Power-Up Serdes No.2 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a000, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a004, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a000, 0x0000be80);
    /* Pre Power-Up Serdes No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000008);
    /* Delay 5mS ----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000001);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000002);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x000000f0, 0x00000004);
    /* Power down lane No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a768, 0x00000000);
    /* Post Power-Up Serdes No.3 */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a400, 0x0000be80);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000018);
    /* Add minimum 16 core clock delay -----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a404, 0x00000028);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0980a400, 0x0000be80);
    /* Stack SP25 XPCS lane configuration -----------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806654, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806698, 0x00000900);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x088066dc, 0x00001200);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806720, 0x00001b00);
    /* PCS de-assert XG port 25 reset & lane configuration ---------------- */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806600, 0x00000047);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806604, 0x000000c0);
    /* De-assert Gig MAC Reset Stacking port 24  --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806000, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806004, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806008, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806048, 0x00000300);
    /* Port24 in-band auto-negotiation enable --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80600c, 0x0000b0ec);
    /* Port24 Move MAC mode to 1G MAC --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0880601c, 0x00000000);
    /* De-assert XG MAC Reset  Stacking port 25  --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806400, 0x00002403);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806404, 0x000002f9);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806408, 0x00003819);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0880641c, 0x00002000);
    /* De-assert Gig MAC Reset Stacking port 26  --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806800, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806804, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806808, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806848, 0x00000300);
    /* Port26 in-band auto-negotiation enable --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a80680c, 0x0000b0ec);
    /* Port26 Move MAC mode to 1G MAC --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0880681c, 0x00000000);
    /* De-assert Gig MAC Reset Stacking port 27  --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c00, 0x00008be5);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c04, 0x00000003);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c08, 0x0000c008);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c48, 0x00000300);
    /* Port27 in-band auto-negotiation enable --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0a806c0c, 0x0000b0ec);
    /* Port27 Move MAC mode to 1G MAC --------------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x08806c1c, 0x00000000);
    /* ----------- Led stream configuration ----------- */
    /* Port[11..0], Led stream ordered by port & Link effect 'on' */
    /* Port[23..12], Led stream ordered by port & Link effect 'on' */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004100, 0x337bff00);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004100, 0x337bff00);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004104, 0x404014b4);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05004104, 0x404014b4);
    /* full duplex green/yellow, for Ports [23..0] */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04804104, 0x55552222);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05804104, 0x55552222);
    /* FD & Lnk ; Class manipulation - Class[3..2], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04804108, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05804108, 0x00000000);
    /* ~Act & Blink ; Class manipulation - Class[4], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0400410c, 0x0000e000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0500410c, 0x0000e000);
    /* HD ; Class manipulation - Class[5], Port[23..0] */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0480410c, 0x00000000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x0580410c, 0x00000000);
    /* PD(0) ; Class manipulation - Class[6], Port[23..0], LED_DATA/STB change should be from negedge of LED_CLK */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04804100, 0x00011000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05804100, 0x00011000);
    /* LMS0,1 HGS debug leds force class link */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x05005110, 0x00000010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04005110, 0x00000010);
    /* PHY0 88E1512 config */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c00012);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02800201);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02808201);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c00000);
    /*PhyErrata */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C000fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01000010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C00000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C000fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01000010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C000fb);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00204099);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00601120);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C000fc);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x002020b0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C000ff);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02204444);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02208064);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002141);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02200108);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002144);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02208c44);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0200214b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02200f90);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0200214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0220ba33);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0200214d);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022039AA);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0200214f);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02208433);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002151);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02202010);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002152);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x022099EB);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002153);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02202f3b);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002154);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0220584e);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002156);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02201223);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002158);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02200fd0);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x0200214c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02200000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02200f16);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02002146);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C000fa);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01A05432);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02C00000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x00009140);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c00000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x02c00000);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01A00007);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01C0003c);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01A04007);
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x04004054, 0x01C00006);
    /* ---------- Device enable ----------  */
    mvOsDelay(1); mvPpWriteReg(MV_PP_DEV0, 0x00000058, 0x04184003);

    return MV_OK;
}
