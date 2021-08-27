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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#ifndef __INCmvBoardEnvLibh
#define __INCmvBoardEnvLibh

/* defines */
/* The below constant macros defines the board I2C EEPROM data offsets */

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "mvSysHwConfig.h"
#include "boardEnv/mvBoardEnvSpec.h"

/* DUART stuff for Tclk detection only */
#define DUART_BAUD_RATE			115200
#define MAX_CLOCK_MARGINE		5000000	/* Maximum detected clock margine */

/* Voice devices assembly modes */
#define DAISY_CHAIN_MODE        1
#define DUAL_CHIP_SELECT_MODE   0
#define INTERRUPT_TO_MPP        1
#define INTERRUPT_TO_TDM        0

#define BOARD_ETH_PORT_NUM  MV_ETH_MAX_PORTS
#define BOARD_MAX_CASCADED_SWITCHES     2
#define BOARD_ETH_SWITCH_PORT_NUM       11
#define BOARD_ETH_MAX_PORT_STRING       32

#define	MV_BOARD_MAX_USB_IF		1
#define MV_BOARD_MAX_MPP		7
#define MV_BOARD_NAME_LEN  		0x20

typedef struct _boardData
{
   MV_U32 magic;
   MV_U16 boardId;
   MV_U8 boardVer;
   MV_U8 boardRev;
   MV_U32 reserved1;
   MV_U32 reserved2;

}BOARD_DATA;

typedef enum _devBoardMppGroupClass
{
	MV_BOARD_MPP_GROUP_1,
	MV_BOARD_MPP_GROUP_2,
	MV_BOARD_MAX_MPP_GROUP
}MV_BOARD_MPP_GROUP_CLASS;

typedef enum _devBoardMppTypeClass
{
	MV_BOARD_AUTO,
	MV_BOARD_TDM,
	MV_BOARD_AUDIO,
	MV_BOARD_RGMII,
	MV_BOARD_GMII,
	MV_BOARD_TS,
	MV_BOARD_MII,
	RSRVD0,
	MV_BOARD_LCD,
	MV_BOARD_OTHER
}MV_BOARD_MPP_TYPE_CLASS;

typedef enum _devBoardModuleIdClass
{
	MV_BOARD_MODULE_TDM_ID = 1,
	MV_BOARD_MODULE_AUDIO_ID,
	MV_BOARD_MODULE_RGMII_ID,
	MV_BOARD_MODULE_GMII_ID,
	MV_BOARD_MODULE_TS_ID,
	MV_BOARD_MODULE_MII_ID,
	MV_BOARD_MODULE_TDM_5CHAN_ID,
	MV_BOARD_MODULE_LCD_ID = 9,
	MV_BOARD_MODULE_OTHER_ID
}MV_BOARD_MODULE_ID_CLASS;

typedef struct _boardMppTypeInfo
{
	MV_BOARD_MPP_TYPE_CLASS	boardMppGroup1;
	MV_BOARD_MPP_TYPE_CLASS	boardMppGroup2;

}MV_BOARD_MPP_TYPE_INFO;

typedef enum _devBoardClass
{
	BOARD_DEV_NOR_FLASH,
	BOARD_DEV_NAND_FLASH,
	BOARD_DEV_SEVEN_SEG,
	BOARD_DEV_FPGA,
	BOARD_DEV_SRAM,
	BOARD_DEV_SPI_FLASH,
	BOARD_DEV_OTHER,
}MV_BOARD_DEV_CLASS;

typedef enum _devTwsiBoardClass
{
	BOARD_TWSI_RTC,
	BOARD_DEV_TWSI_EXP,
	BOARD_DEV_TWSI_SATR,
	BOARD_TWSI_AUDIO_DEC,
	BOARD_TWSI_OTHER
}MV_BOARD_TWSI_CLASS;

typedef enum _devGppBoardClass
{
	BOARD_GPP_RTC,
	BOARD_GPP_MV_SWITCH,
	BOARD_GPP_USB_VBUS,
	BOARD_GPP_USB_VBUS_EN,
	BOARD_GPP_USB_OC,
	BOARD_GPP_USB_HOST_DEVICE,
	BOARD_GPP_REF_CLCK,
	BOARD_GPP_VOIP_SLIC,
	BOARD_GPP_LIFELINE,
	BOARD_GPP_BUTTON,
	BOARD_GPP_TS_BUTTON_C,
	BOARD_GPP_TS_BUTTON_U,
	BOARD_GPP_TS_BUTTON_D,
	BOARD_GPP_TS_BUTTON_L,
	BOARD_GPP_TS_BUTTON_R,
	BOARD_GPP_POWER_BUTTON,
	BOARD_GPP_RESTOR_BUTTON,
	BOARD_GPP_WPS_BUTTON,
	BOARD_GPP_HDD0_POWER,
	BOARD_GPP_HDD1_POWER,
	BOARD_GPP_FAN_POWER,
	BOARD_GPP_RESET,
	BOARD_GPP_POWER_ON_LED,
	BOARD_GPP_HDD_POWER,
    BOARD_GPP_SDIO_POWER,
    BOARD_GPP_SDIO_DETECT,
    BOARD_GPP_SDIO_WP,
	BOARD_GPP_SWITCH_PHY_INT,
	BOARD_GPP_TSU_DIRCTION,
	BOARD_GPP_OTHER
}MV_BOARD_GPP_CLASS;

typedef struct _devCsInfo
{
    MV_U8		deviceCS;
    MV_U32		params;
    MV_U32		devClass;	/* MV_BOARD_DEV_CLASS */
    MV_U8		devWidth;

}MV_DEV_CS_INFO;

typedef struct _boardSwitchPort
{
    MV_32   qdSwitchNum;                                /* Switch index, e.g., 0, 1 */
    MV_32   qdPortNum;                                  /* Switch physical port number */
    char    portStr[BOARD_ETH_MAX_PORT_STRING];         /* String to print for link change messages */
}MV_BOARD_SWITCH_PORT;

typedef struct _boardSwitchInfo
{
    MV_32   numSwitchesOnPort;    /* number of Switches on this port: 1 means only one Switch is connected, 2 means two cascaded Switches on this port */
    MV_32	linkStatusIrq[BOARD_MAX_CASCADED_SWITCHES];         /* link indication IRQ (MPP) number, or -1 for polling */
    MV_BOARD_SWITCH_PORT    qdPort[BOARD_ETH_SWITCH_PORT_NUM];  /* map logical port number to switch index and physical switch port number */
    MV_32	qdCpuPort[BOARD_MAX_CASCADED_SWITCHES];             /* Switch port directed to the CPU (GbE) */
    MV_32   qdInterconnectPort[BOARD_MAX_CASCADED_SWITCHES];    /* Switch port connected to another Switch. Assumes only 2 cascaded Switches */
    MV_32	smiScanMode;                                        /* 1 for SMI_MANUAL_MODE, 2 for SMI_MULTI_ADDR_MODE, 0 otherwise */
    MV_U8   switchSmiAddr[BOARD_MAX_CASCADED_SWITCHES];         /* Switch SMI address */
}MV_BOARD_SWITCH_INFO;

typedef struct _boardLedInfo
{
	MV_U8	activeLedsNumber;
	MV_U8	ledsPolarity;	/* '0' or '1' to turn on led */
	MV_U8*	gppPinNum; 	/* Pointer to GPP values */

}MV_BOARD_LED_INFO;

typedef struct _boardGppInfo
{
	MV_BOARD_GPP_CLASS	devClass;
	MV_U8	gppPinNum;

}MV_BOARD_GPP_INFO;

typedef struct _boardTwsiInfo
{
	MV_BOARD_TWSI_CLASS	devClass;
	MV_U8	twsiDevAddr;
	MV_U8	twsiDevAddrType;

}MV_BOARD_TWSI_INFO;

typedef enum _boardMacSpeed
{
	BOARD_MAC_SPEED_10M,
	BOARD_MAC_SPEED_100M,
	BOARD_MAC_SPEED_1000M,
	BOARD_MAC_SPEED_AUTO,

}MV_BOARD_MAC_SPEED;

typedef struct _boardMacInfo
{
	MV_BOARD_MAC_SPEED  boardMacSpeed;
	MV_U8               boardEthSmiAddr;

}MV_BOARD_MAC_INFO;

typedef struct _boardMppInfo
{
	MV_U32		mppGroup[MV_BOARD_MAX_MPP];

}MV_BOARD_MPP_INFO;

typedef struct _mvPortDsaInfo
{
	MV_BOOL		useDsaTag;
	MV_U32		dsaTagLen;
} MV_PORT_DSA_INFO;

typedef struct _boardInfo
{
	char 			   	boardName[MV_BOARD_NAME_LEN];
	MV_U8				numBoardMppTypeValue;
	MV_BOARD_MPP_TYPE_INFO*		pBoardMppTypeValue;
	MV_U8				numBoardMppConfigValue;
	MV_BOARD_MPP_INFO*	pBoardMppConfigValue;
    MV_U32				intsGppMaskLow;
	MV_U32				intsGppMaskHigh;
	MV_U8				numBoardDeviceIf;
    MV_DEV_CS_INFO*		pDevCsInfo;
	MV_U8				numBoardTwsiDev;
	MV_BOARD_TWSI_INFO*	pBoardTwsiDev;
	MV_U8				numBoardMacInfo;
	MV_BOARD_MAC_INFO*	pBoardMacInfo;
	MV_U8				numBoardGppInfo;
	MV_BOARD_GPP_INFO*	pBoardGppInfo;
    MV_U8				activeLedsNumber;
	MV_U8*				pLedGppPin;
	MV_U8				ledsPolarity;	/* '0' or '1' to turn on led */
	/* GPP values */
	MV_U32				gppOutEnValLow;
	MV_U32				gppOutEnValHigh;
	MV_U32				gppOutValLow;
	MV_U32				gppOutValHigh;
	MV_U32				gppPolarityValLow;
	MV_U32				gppPolarityValHigh;

	/* Switch Configuration */
	MV_BOARD_SWITCH_INFO*		pSwitchInfo;
	MV_U32				nandFlashReadParams;
	MV_U32				nandFlashWriteParams;
	MV_U32				nandFlashControl;

	/* General DSA tag support */
	MV_PORT_DSA_INFO		*portDsaInfo;
}MV_BOARD_INFO;

MV_VOID 	mvBoardEnvInit(MV_VOID);
MV_VOID     mvBoardIdSet(MV_U32 boardId);
MV_U32      mvBoardIdGet(MV_VOID);
MV_STATUS	mvBoardNameGet(char *pNameBuff);
MV_32      	mvBoardPhyAddrGet(MV_U32 ethPortNum);
MV_BOARD_MAC_SPEED      mvBoardMacSpeedGet(MV_U32 ethPortNum);

MV_U32 mvBoardDsaTagLenGet(MV_U32 port);
MV_BOOL mvBoardUseDsaTagOnPort(MV_U32 port);

/* Switch related: */
MV_32       mvBoardNumSwitchesOnPortGet(MV_U32 ethPortNum);
MV_32		mvBoardLinkStatusIrqGet(MV_U32 ethPortNum, MV_U32 switchIndex);
MV_32   	mvBoardSwitchPortGet(MV_U32 ethPortNum, MV_U8 logicalPortNum, MV_32 *switchIndex, MV_32 *portNum, char *portName);
MV_32		mvBoardSwitchCpuPortGet(MV_U32 ethPortNum, MV_U32 switchIndex);
MV_32       mvBoardSwitchInterconnectPortGet(MV_U32 ethPortNum, MV_U32 switchIndex);
MV_32		mvBoardSmiScanModeGet(MV_U32 ethPortNum);
MV_32       mvBoardSwitchSmiAddrGet(MV_U32 ethPortNum, MV_U32 switchIndex);
MV_BOOL     mvBoardIsSwitchConnected(MV_U32 ethPortNum);

MV_BOOL     mvBoardIsPortInSgmii(MV_U32 ethPortNum);
MV_BOOL 	mvBoardIsPortInGmii(MV_VOID);
MV_32      	mvBoardMppGet(MV_U32 mppGroupNum);

MV_U8 		mvBoardTwsiExpAddrGet(MV_U32 index);
MV_U8 		mvBoardTwsiSatRAddrTypeGet(MV_U32 index);
MV_U8 		mvBoardTwsiSatRAddrGet(MV_U32 index);
MV_U8 		mvBoardTwsiExpAddrTypeGet(MV_U32 index);
MV_BOARD_MODULE_ID_CLASS 	mvBoarModuleTypeGet(MV_BOARD_MPP_GROUP_CLASS devClass);
MV_BOARD_MPP_TYPE_CLASS 	mvBoardMppGroupTypeGet(MV_BOARD_MPP_GROUP_CLASS mppGroupClass);
MV_VOID 	mvBoardMppGroupTypeSet(MV_BOARD_MPP_GROUP_CLASS mppGroupClass,
						MV_BOARD_MPP_TYPE_CLASS mppGroupType);
MV_VOID 	mvBoardMppGroupIdUpdate(MV_VOID);
MV_VOID 	mvBoardMppMuxSet(MV_VOID);
MV_VOID 	mvBoardTdmMppSet(MV_32 chType);
MV_32 		mvBoardTdmSpiModeGet(MV_VOID);
MV_32		mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS class, MV_U32 index);

#endif /* __INCmvBoardEnvLibh */
