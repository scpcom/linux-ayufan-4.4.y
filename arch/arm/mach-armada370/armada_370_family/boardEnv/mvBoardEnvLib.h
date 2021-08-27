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

    *	Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *	Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *	Neither the name of Marvell nor the names of its contributors may be
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* defines */
/* The below constant macros defines the board I2C EEPROM data offsets */

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "mvSysHwConfig.h"
#include "boardEnv/mvBoardEnvSpec.h"
#include "twsi/mvTwsi.h"

/* DUART stuff for Tclk detection only */
#define DUART_BAUD_RATE			115200
#define MAX_CLOCK_MARGINE		5000000	/* Maximum detected clock margine */

/* Voice devices assembly modes */
#define DAISY_CHAIN_MODE		1
#define DUAL_CHIP_SELECT_MODE		0
#define INTERRUPT_TO_MPP		1
#define INTERRUPT_TO_TDM		0

#define BOARD_ETH_PORT_NUM		MV_ETH_MAX_PORTS
#define BOARD_ETH_SWITCH_PORT_NUM	5

#define	MV_BOARD_MAX_USB_IF		2
#define MV_BOARD_MAX_MPP		9	/* number of MPP conf registers */
#define MV_BOARD_NAME_LEN  		0x20

/* EPPROM Modules detection information */

#define MV_BOARD_MODULES_ADDR_TYPE			ADDR7_BIT

#define MV_BOARD_DEVICE_CON_ADDR			0x20
#define MV_BOARD_EEPROM_MODULE_ADDR			0x21
#define MV_BOARD_GIGA_CON_GMII_ADDR			0x22
#define MV_BOARD_GIGA_CON_ADDR				0x26
#define MV_BOARD_SERDES_CON_ADDR			0x27

#define MV_BOARD_TDM_GMII_MODULE_TDM_ID		0x1
#define MV_BOARD_TDM_GMII_MODULE_GMII_ID	0x4
#define MV_BOARD_SWITCH_MODULE_ID			0xE
#define MV_BOARD_I2S_SPDIF_MODULE_ID		0x2
#define MV_BOARD_NAND_SDIO_MODULE_ID		0xF
#define MV_BOARD_MODULE_ID_MASK				0xF

/* Mapping of SW IO Expander bits:
** 0-1: Sata0 Selection
** 	0x0: SATA-0 not connected.
** 	0x1: SATA-0 on Lane-0
** 	0x2: SATA-0 on Lane-2
** 2:	Sata1 Selection
** 	0x0: SATA-1 not connected.
** 	0x1: SATA-1 on Lane-3
** 3-4: PCI-E Selection
** 	0x0: No PCIE
** 	0x1: PCIE-0 on Lane 0
** 	0x2: PCIE-1 on Lane 1
** 	0x3: PCIE-0 & PCIE-1
** 5: On board SDIO / RGMII0
** 	0x0: SDIO disabled.
** 	0x1: SDIO enabled.
*/
#define MV_BOARD_CFG_SATA0_MODE(cfg)	(cfg & 0x3)
#define MV_BOARD_CFG_SATA1_MODE(cfg)	((cfg >> 2) & 0x1)
#define MV_BOARD_CFG_PCIE_MODE(cfg)		((cfg >> 3) & 0x3)
#define MV_BOARD_CFG_SDIO_MODE(cfg)		((cfg >> 5) & 0x1)

typedef struct _boardData {
	MV_U32 magic;
	MV_U16 boardId;
	MV_U8 boardVer;
	MV_U8 boardRev;
	MV_U32 reserved1;
	MV_U32 reserved2;
} BOARD_DATA;

typedef enum _devBoardMppGroupClass {
	MV_BOARD_MPP_GROUP_1,
	MV_BOARD_MPP_GROUP_2,
	MV_BOARD_MPP_GROUP_3,
	MV_BOARD_MAX_MPP_GROUP
} MV_BOARD_MPP_GROUP_CLASS;

typedef enum _devBoardMppTypeClass {
	MV_BOARD_AUTO	= 0x0,
	MV_BOARD_TDM	= 0x01,
	MV_BOARD_I2S	= 0x02,
	MV_BOARD_GMII0	= 0x04,
	MV_BOARD_SDIO	= 0x08,
	MV_BOARD_RGMII0	= 0x10,
	MV_BOARD_RGMII1 = 0x20,
	MV_BOARD_OTHER  = 0xFFFFFFFF
} MV_BOARD_MPP_TYPE_CLASS;

typedef enum {
	MV_BOARD_NONE	= 0x0001,
	MV_BOARD_SGMII0 = 0x0002,
	MV_BOARD_SGMII1 = 0x0004,
	MV_BOARD_PEX0	= 0x0008,
	MV_BOARD_PEX1	= 0x0010,
	MV_BOARD_SATA0	= 0x0020,
	MV_BOARD_SATA1	= 0x0040,
	MV_BOARD_UNKNOWN = 0x80000000
} MV_BOARD_OTHER_TYPE_CLASS;

typedef struct _boardModuleTypeInfo {
	MV_U32 boardMppGrp1Mod;
	MV_U32 boardMppGrp2Mod;
} MV_BOARD_MODULE_TYPE_INFO;

typedef enum _devBoardClass {
	BOARD_DEV_NOR_FLASH,
	BOARD_DEV_NAND_FLASH,
	BOARD_DEV_SEVEN_SEG,
	BOARD_DEV_FPGA,
	BOARD_DEV_SRAM,
	BOARD_DEV_SPI_FLASH,
	BOARD_DEV_OTHER
} MV_BOARD_DEV_CLASS;

typedef enum _devTwsiBoardClass {
	BOARD_TWSI_RTC,
	BOARD_DEV_TWSI_EXP,
	BOARD_DEV_TWSI_SATR,
	BOARD_TWSI_MUX,
	BOARD_TWSI_OTHER
} MV_BOARD_TWSI_CLASS;

typedef enum _devGppBoardClass {
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
	BOARD_GPP_CONF,
	BOARD_GPP_OTHER
} MV_BOARD_GPP_CLASS;

typedef struct _devCsInfo {
	MV_U8 deviceCS;
	MV_U32 params;
	MV_U32 devClass;	/* MV_BOARD_DEV_CLASS */
	MV_U8 devWidth;
	MV_U8 busWidth;
} MV_DEV_CS_INFO;

typedef struct _boardSwitchInfo {
	MV_32 switchIrq;
	MV_32 switchPort[BOARD_ETH_SWITCH_PORT_NUM];
	MV_32 cpuPort;
	MV_32 connectedPort[MV_ETH_MAX_PORTS];
	MV_32 smiScanMode;
	MV_32 quadPhyAddr;
	MV_U32 forceLinkMask; /* Bitmask of switch ports to have force link (1Gbps) */
} MV_BOARD_SWITCH_INFO;

typedef struct _boardLedInfo {
	MV_U8 activeLedsNumber;
	MV_U8 ledsPolarity;	/* '0' or '1' to turn on led */
	MV_U8 *gppPinNum;	/* Pointer to GPP values */
} MV_BOARD_LED_INFO;

typedef struct _boardGppInfo {
	MV_BOARD_GPP_CLASS devClass;
	MV_U8 gppPinNum;
} MV_BOARD_GPP_INFO;

typedef struct _boardTwsiInfo {
	MV_BOARD_TWSI_CLASS devClass;
	MV_U8 twsiDevAddr;
	MV_U8 twsiDevAddrType;
} MV_BOARD_TWSI_INFO;

typedef enum _boardMacSpeed {
	BOARD_MAC_SPEED_10M,
	BOARD_MAC_SPEED_100M,
	BOARD_MAC_SPEED_1000M,
	BOARD_MAC_SPEED_AUTO,
} MV_BOARD_MAC_SPEED;

typedef struct _boardMacInfo {
	MV_BOARD_MAC_SPEED boardMacSpeed;
	MV_U8 boardEthSmiAddr;
	MV_U16	LinkCryptPortAddr;
	MV_U8	boardEthSmiAddr0;
} MV_BOARD_MAC_INFO;

typedef struct _boardMppInfo {
	MV_U32 mppGroup[MV_BOARD_MAX_MPP];
} MV_BOARD_MPP_INFO;

typedef struct {
	MV_U8 spiCs;
} MV_BOARD_TDM_INFO;

typedef struct {
	MV_U8 spiId;
} MV_BOARD_TDM_SPI_INFO;

typedef struct _boardPexInfo {
	MV_PEX_UNIT_CFG 	pexUnitCfg[MV_PEX_MAX_UNIT];
	MV_U32			boardPexIfNum;
} MV_BOARD_PEX_INFO;

typedef enum {
	BOARD_TDM_SLIC_880 = 0,
	BOARD_TDM_SLIC_792,
	BOARD_TDM_SLIC_3215,
	BOARD_TDM_SLIC_OTHER,
	BOARD_TDM_SLIC_COUNT
} MV_BOARD_TDM_SLIC_TYPE;

typedef struct _boardInfo {
	char boardName[MV_BOARD_NAME_LEN];
	MV_BOOL enableModuleScan;
	MV_U8 numBoardMppTypeValue;
	MV_BOARD_MODULE_TYPE_INFO *pBoardModTypeValue;
	MV_U8 numBoardMppConfigValue;
	MV_BOARD_MPP_INFO *pBoardMppConfigValue;
	MV_BOARD_PEX_INFO boardPexInfo;	/* filled in runtime */
	MV_U32 intsGppMaskLow;
	MV_U32 intsGppMaskMid;
	MV_U32 intsGppMaskHigh;
	MV_U8 numBoardDeviceIf;
	MV_DEV_CS_INFO *pDevCsInfo;
	MV_U8 numBoardTwsiDev;
	MV_BOARD_TWSI_INFO *pBoardTwsiDev;
	MV_U8 numBoardMacInfo;
	MV_BOARD_MAC_INFO *pBoardMacInfo;
	MV_U8 numBoardGppInfo;
	MV_BOARD_GPP_INFO *pBoardGppInfo;
	MV_U8 activeLedsNumber;
	MV_U8 *pLedGppPin;
	MV_U8 ledsPolarity;	/* '0' or '1' to turn on led */

	MV_U8	pmuPwrUpPolarity;
	MV_U32	pmuPwrUpDelay;
	/* GPP values */
	MV_U32 gppOutEnValLow;
	MV_U32 gppOutEnValMid;
	MV_U32 gppOutEnValHigh;
	MV_U32 gppOutValLow;
	MV_U32 gppOutValMid;
	MV_U32 gppOutValHigh;
	MV_U32 gppPolarityValLow;
	MV_U32 gppPolarityValMid;
	MV_U32 gppPolarityValHigh;

	/* External Switch Configuration */
	MV_BOARD_SWITCH_INFO *pSwitchInfo;
	MV_U32 switchInfoNum;

	/* TDM configuration */
	/* We hold a different configuration array for each possible slic that
	 ** can be connected to board.
	 ** When modules are scanned, then we select the index of the relevant
	 ** slic's information array.
	 ** For RD and Customers boards we only need to initialize a single
	 ** entry of the arrays below, and set the boardTdmInfoIndex to 0.
	 */
	MV_U8 numBoardTdmInfo[BOARD_TDM_SLIC_COUNT];
	MV_BOARD_TDM_INFO *pBoardTdmInt2CsInfo[BOARD_TDM_SLIC_COUNT];
	MV_16 boardTdmInfoIndex;
	MV_BOARD_TDM_SPI_INFO *pBoardTdmSpiInfo;

	/* NAND init params */
	MV_U32 nandFlashReadParams;
	MV_U32 nandFlashWriteParams;
	MV_U32 nandFlashControl;
	MV_U32 norFlashReadParams;
	MV_U32 norFlashWriteParams;

} MV_BOARD_INFO;

/* Functions Decleraion */

/* Board General */
MV_VOID mvBoardEnvInit(MV_VOID);
MV_VOID mvBoardReset(MV_VOID);
MV_U16 mvBoardModelGet(MV_VOID);
MV_U16 mvBoardRevGet(MV_VOID);
MV_STATUS mvBoardNameGet(char *pNameBuff);
MV_U32 mvBoardTclkGet(MV_VOID);
MV_U32 mvBoardSysClkGet(MV_VOID);
MV_BOOL mvBoardSpecInitGet(MV_U32 *regOff, MV_U32 *data);

/* GPIO */
MV_32 mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS gppClass, MV_U32 index);
MV_32 mvBoardResetGpioPinGet(MV_VOID);
MV_32 mvBoardSDIOGpioPinGet(MV_BOARD_GPP_CLASS type);
MV_32 mvBoardUSBVbusGpioPinGet(MV_32 devId);
MV_32 mvBoardUSBVbusEnGpioPinGet(MV_32 devId);
MV_U32 mvBoardGpioIntMaskGet(MV_U32 gppGrp);
MV_32 mvBoardMppGet(MV_U32 mppGroupNum);
MV_U32 mvBoardGppConfigGet(void);
MV_VOID mvBoardDebugLed(MV_U32 hexNum);
MV_U32 mvBoardDebugLedNumGet(MV_U32 boardId);
MV_VOID mvBoardBitMaskConfigSet(MV_U32 config);

/* Networking */
MV_BOOL mvBoardIsSwitchConnected(void);
MV_BOOL mvBoardIsGMIIConnected(void);
MV_BOOL mvBoardIsGbEPortConnected(MV_U32 ethPortNum);
MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum);
MV_32 mvBoardQuadPhyAddr0Get(MV_U32 ethPortNum);
MV_32 mvBoardPhyLinkCryptPortAddrGet(MV_U32 ethPortNum);
MV_BOARD_MAC_SPEED mvBoardMacSpeedGet(MV_U32 ethPortNum);
MV_32 mvBoardSmiScanModeGet(MV_U32 switchIdx);
MV_BOOL mvBoardIsPortInGmii(MV_U32 ethPortNum);
MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum);
MV_BOOL mvBoardIsPortInRgmii(MV_U32 ethPortNum);
MV_32 mvBoardSwitchPortGet(MV_U32 switchIdx, MV_U32 boardPortNum);
MV_32 mvBoardSwitchConnectedPortGet(MV_U32 ethPortNum);
MV_32 mvBoardSwitchPortMap(MV_U32 switchIdx, MV_U32 switchPortNum);
MV_32 mvBoardSwitchCpuPortGet(MV_U32 switchIdx);
MV_BOOL mvBoardIsQsgmiiModuleConnected(MV_VOID);
MV_32 mvBoardGePhySwitchPortGet(MV_VOID);
MV_32 mvBoardRgmiiASwitchPortGet(MV_VOID);
MV_32 mvBoardSwitchIrqGet(MV_VOID);
MV_32 mvBoardSwitchCpuPortGet(MV_U32 switchIdx);
MV_32 mvBoardSmiScanModeGet(MV_U32 switchIdx);

/* TDM */
MV_32 mvBoardTdmSpiModeGet(MV_VOID);
MV_U8 mvBoardTdmDevicesCountGet(void);
MV_U8 mvBoardTdmSpiCsGet(MV_U8 devId);
MV_U8 mvBoardTdmSpiIdGet(MV_VOID);

/* Device Bus */
MV_32 mvBoardGetDevicesNumber(MV_BOARD_DEV_CLASS devClass);
MV_32 mvBoardGetDeviceBaseAddr(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);
MV_32 mvBoardGetDeviceBusWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);
MV_32 mvBoardGetDeviceWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);
MV_32 mvBoardGetDeviceWinSize(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);
MV_U32 boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);
MV_32 mvBoardNandWidthGet(void);

/* Twsi */
MV_U8 mvBoardTwsiAddrTypeGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index);
MV_U8 mvBoardTwsiAddrGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index);
MV_U32 mvBoardIdGet(MV_VOID);
MV_VOID mvBoardIdSet(MV_VOID);
MV_U8 mvBoardTwsiSatRGet(MV_U8 devNum, MV_U8 regNum);
MV_STATUS mvBoardTwsiSatRSet(MV_U8 devNum, MV_U8 regNum, MV_U8 regVal);
MV_U8 mvBoardFabFreqGet(MV_VOID);
MV_STATUS mvBoardFabFreqSet(MV_U8 freqVal);
MV_U8 mvBoardCpuFreqGet(MV_VOID);
MV_STATUS mvBoardCpuFreqSet(MV_U8 freqVal);
MV_U8 mvBoardBootDevGet(MV_VOID);
MV_STATUS mvBoardBootDevSet(MV_U8 val);
MV_U8 mvBoardBootDevWidthGet(MV_VOID);
MV_STATUS mvBoardBootDevWidthSet(MV_U8 val);
MV_U8 mvBoardCpu0CoreModeGet(MV_VOID);
MV_STATUS mvBoardCpu0CoreModeSet(MV_U8 val);

/* Modules and Serdeses */
MV_STATUS mvBoardMppModulesScan(void);
MV_STATUS mvBoardUpdateMppAfterScan(void);
MV_BOOL mvBoardIsModScanEnabled(void);
MV_STATUS mvBoardUpdateEthAfterScan(void);
MV_U32 mvBoardMppModulesCfgGet(MV_U8 group);
MV_VOID mvBoardMppModuleTypePrint(MV_VOID);
MV_VOID mvBoardOtherModuleTypePrint(MV_VOID);
MV_U32 mvBoardSerdesModeGet(void);

/* PEX */
MV_BOOL	mvBoardIsPciEConnected(MV_U32 pcieIdx);
MV_BOARD_PEX_INFO *mvBoardPexInfoGet(void);
MV_U16 mvBoardPexCapabilityGet(MV_VOID);
MV_STATUS mvBoardPexCapabilitySet(MV_U16 conf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCmvBoardEnvLibh */
