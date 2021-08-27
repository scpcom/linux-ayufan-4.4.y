#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "cntmr/mvCntmr.h"
#include "gpp/mvGpp.h"
#include "twsi/mvTwsi.h"
#include "pex/mvPex.h"
#include "pci/mvPci.h"
#include "device/mvDevice.h"

#if defined(CONFIG_MV_ETH_NETA)
#include "neta/gbe/mvEthRegs.h"
#elif defined(CONFIG_MV_ETH_PP2)
#include "pp2/gbe/mvPp2GbeRegs.h"
#endif

#include "gpp/mvGppRegs.h"

#undef MV_DEBUG
#ifdef MV_DEBUG
#define DB(x)   x
#else
#define DB(x)
#endif

extern MV_BOARD_INFO *marvellBoardInfoTbl[];
extern MV_BOARD_INFO *customerBoardInfoTbl[];
extern MV_BOARD_SATR_INFO boardSatrInfo[];
MV_BOARD_CONFIG_TYPE_INFO boardConfigTypesInfo[] = MV_BOARD_CONFIG_INFO;

static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);
 
static MV_BOARD_INFO *board = (MV_BOARD_INFO *)-1;

MV_U32 mvBoardIdIndexGet(MV_U32 boardId)
{
 
	return boardId & (MARVELL_BOARD_ID_BASE - 1);
}

MV_VOID mvBoardEnvInit(MV_VOID)
{
	MV_U32 nandDev;
	MV_U32 norDev;

	mvBoardSet(mvBoardIdGet());
	MV_U32 syncCtrl = 0;

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
	 
#else
	nandDev = boardGetDevCSNum(0, BOARD_DEV_NAND_FLASH);
	if (nandDev != 0xFFFFFFFF) {
		 
		MV_REG_WRITE(DEV_BANK_PARAM_REG(nandDev), board->nandFlashReadParams);
		MV_REG_WRITE(DEV_BANK_PARAM_REG_WR(nandDev), board->nandFlashWriteParams);
		MV_REG_WRITE(DEV_NAND_CTRL_REG, board->nandFlashControl);
		 
		syncCtrl |= SYNC_CTRL_READY_POL(nandDev);
	}

	norDev = boardGetDevCSNum(0, BOARD_DEV_NOR_FLASH);
	if (norDev != 0xFFFFFFFF) {
		 
		MV_REG_WRITE(DEV_BANK_PARAM_REG(norDev), board->norFlashReadParams);
		MV_REG_WRITE(DEV_BANK_PARAM_REG_WR(norDev), board->norFlashWriteParams);
		 
		syncCtrl |= SYNC_CTRL_READY_IGNORE(norDev);
	}

	if (nandDev != 0xFFFFFFFF || norDev != 0xFFFFFFFF) {
		 
		syncCtrl |= 0x1;
		 
		MV_REG_WRITE(DEV_BUS_SYNC_CTRL, syncCtrl);
	}

	MV_REG_WRITE(GPP_DATA_OUT_REG(0), board->gppOutValLow);
	MV_REG_WRITE(GPP_DATA_OUT_REG(1), board->gppOutValMid);
	MV_REG_WRITE(GPP_DATA_OUT_REG(2), board->gppOutValHigh);

	mvGppPolaritySet(0, 0xFFFFFFFF, board->gppPolarityValLow);
	mvGppPolaritySet(1, 0xFFFFFFFF, board->gppPolarityValMid);
	mvGppPolaritySet(2, 0xFFFFFFFF, board->gppPolarityValHigh);

	mvGppTypeSet(0, 0xFFFFFFFF, board->gppOutEnValLow);
	mvGppTypeSet(1, 0xFFFFFFFF, board->gppOutEnValMid);
	mvGppTypeSet(2, 0xFFFFFFFF, board->gppOutEnValHigh);
#endif
}

MV_U16 mvBoardModelGet(MV_VOID)
{
	return mvBoardIdIndexGet(mvBoardIdGet()) >> 16;
}

MV_U16 mvBoardRevGet(MV_VOID)
{
	return mvBoardIdIndexGet(mvBoardIdGet()) & 0xFFFF;
}

MV_STATUS mvBoardNameGet(char *pNameBuff, MV_U32 size)
{
	mvOsSNPrintf(pNameBuff, size, "%s", board->boardName);
	return MV_OK;
}

MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum)
{
	MV_U32 ethComplex = mvBoardEthComplexConfigGet();
	if (ethPortNum == 1 && (ethComplex & MV_ETHCOMP_GE_MAC1_2_PON_ETH_SERDES ||
			     (ethComplex & MV_ETHCOMP_GE_MAC1_2_PON_ETH_SERDES_SFP)))
		return MV_TRUE;

	return MV_FALSE;
}

MV_BOOL mvBoardIsPortInGmii(MV_U32 ethPortNum)
{
	return MV_FALSE;
}

MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum)
{
	if (ethPortNum >= board->numBoardMacInfo) {
		DB(mvOsPrintf("%s: Error: invalid ethPortNum (%d)\n", __func__, ethPortNum));
		return MV_ERROR;
	}

	return board->pBoardMacInfo[ethPortNum].boardEthSmiAddr;
}

MV_VOID mvBoardPhyAddrSet(MV_U32 ethPortNum, MV_U32 smiAddr)
{
	if (ethPortNum >= board->numBoardMacInfo) {
		DB(mvOsPrintf("%s: Error: invalid ethPortNum (%d)\n", __func__, ethPortNum));
		return;
	}

	board->pBoardMacInfo[ethPortNum].boardEthSmiAddr = smiAddr;
}
 
MV_32 mvBoardQuadPhyAddr0Get(MV_U32 ethPortNum)
{
	if (ethPortNum >= board->numBoardMacInfo) {
		DB(mvOsPrintf("%s: Error: invalid ethPortNum (%d)\n", __func__, ethPortNum));
		return MV_ERROR;
	}

	return board->pBoardMacInfo[ethPortNum].boardEthSmiAddr0;
}

MV_BOARD_SPEC_INIT *mvBoardSpecInitGet(MV_VOID)
{
	return board->pBoardSpecInit;
}

MV_BOARD_MAC_SPEED mvBoardMacSpeedGet(MV_U32 ethPortNum)
{
	if (ethPortNum >= board->numBoardMacInfo) {
		mvOsPrintf("%s: Error: wrong eth port (%d)\n", __func__, ethPortNum);
		return BOARD_MAC_SPEED_100M;
	}

	return board->pBoardMacInfo[ethPortNum].boardMacSpeed;
}

MV_BOOL mvBoardIsPortLoopback(MV_U32 ethPortNum)
{
	return (ethPortNum == 2);
}

MV_U32 mvBoardTclkGet(MV_VOID)
{
	MV_U32 tclk;
	tclk = (MV_REG_READ(MPP_SAMPLE_AT_RESET(1)));
	tclk = ((tclk & 0x400000) >> 22);
	switch (tclk) {
	case 0:
		return MV_BOARD_TCLK_166MHZ;
	case 1:
		return MV_BOARD_TCLK_200MHZ;
	default:
		return MV_BOARD_TCLK_200MHZ;
	}
}

MV_U32 mvBoardSysClkGet(MV_VOID)
{
	MV_FREQ_MODE freqMode;
	if (MV_ERROR != mvCtrlCpuDdrL2FreqGet(&freqMode))
		return (MV_U32)(1000000 * freqMode.ddrFreq);
	else
		return MV_ERROR;
}

MV_U32 mvBoardDebugLedNumGet(MV_U32 boardId)
{
	return board->activeLedsNumber;
}

MV_VOID mvBoardDebugLed(MV_U32 hexNum)
{
	 
}

MV_32 mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS gppClass, MV_U32 index)
{
	MV_U32 i, indexFound = 0;

	for (i = 0; i < board->numBoardGppInfo; i++) {
		if (board->pBoardGppInfo[i].devClass == gppClass) {
			if (indexFound == index)
				return (MV_U32)board->pBoardGppInfo[i].gppPinNum;
			else
				indexFound++;
		}
	}

	return MV_ERROR;
}

MV_VOID mvBoardReset(MV_VOID)
{
	MV_32 resetPin;

	resetPin = mvBoardResetGpioPinGet();
	if (resetPin != MV_ERROR)
		MV_REG_BIT_RESET(GPP_DATA_OUT_REG((int)(resetPin/32)), (1 << (resetPin % 32)));
	else
	{
		 
		MV_REG_BIT_SET( CPU_RSTOUTN_MASK_REG , BIT0);
		MV_REG_BIT_SET( CPU_SYS_SOFT_RST_REG , BIT0);
	}
}

MV_32 mvBoardResetGpioPinGet(MV_VOID)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_RESET, 0);
}

MV_32 mvBoardSDIOGpioPinGet(MV_BOARD_GPP_CLASS type)
{
	if (type != BOARD_GPP_SDIO_POWER &&
	    type != BOARD_GPP_SDIO_DETECT &&
	    type != BOARD_GPP_SDIO_WP)
		return MV_FAIL;

	return mvBoarGpioPinNumGet(type, 0);
}

MV_32 mvBoardUSBVbusGpioPinGet(MV_32 devId)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_USB_VBUS, devId);
}

MV_32 mvBoardUSBVbusEnGpioPinGet(MV_32 devId)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_USB_VBUS_EN, devId);
}

MV_U32 mvBoardGpioIntMaskGet(MV_U32 gppGrp)
{
	switch (gppGrp) {
	case 0:
		return board->intsGppMaskLow;
		break;
	case 1:
		return board->intsGppMaskMid;
		break;
	case 2:
		return board->intsGppMaskHigh;
		break;
	default:
		return MV_ERROR;
	}
}

MV_U32 mvBoardSlicUnitTypeGet(MV_VOID)
{
	return board->pBoardModTypeValue->boardMppSlic;
}

MV_VOID mvBoardSlicUnitTypeSet(MV_U32 slicType)
{
	board->pBoardModTypeValue->boardMppSlic = slicType;
}

MV_U8 mvBoardIoExpValGet(MV_U8 regNum, MV_U8 expanderNum, MV_U8 offset)
{
	MV_U8 val, mask;

	if (mvBoardTwsiGet(BOARD_DEV_TWSI_IO_EXPANDER, expanderNum, regNum, &val) != MV_OK) {
		mvOsPrintf("%s: Error: Read from IO Expander at 0x%x failed\n", __func__
			   , mvBoardTwsiAddrGet(BOARD_DEV_TWSI_IO_EXPANDER, expanderNum));
		return (MV_U8)MV_ERROR;
	}

	mask = (1 << offset);
	return (val & mask) >> offset;
}

MV_STATUS mvBoardIoExpValSet(MV_U8 regNum, MV_U8 expanderNum, MV_U8 offset, MV_U8 value)
{
	MV_U8 readVal, configVal;

	if (mvBoardTwsiGet(BOARD_DEV_TWSI_IO_EXPANDER, expanderNum,
					regNum, &readVal) != MV_OK) {
		mvOsPrintf("%s: Error: Read from IO Expander failed\n", __func__);
		return MV_ERROR;
	}

	if (mvBoardTwsiGet(BOARD_DEV_TWSI_IO_EXPANDER, expanderNum,
					regNum + 6, &configVal) != MV_OK) {
		mvOsPrintf("%s: Error: Read Configuration from IO Expander failed\n", __func__);
		return MV_ERROR;
	}

	configVal &= ~(1 << offset);	 
	if (mvBoardTwsiSet(BOARD_DEV_TWSI_IO_EXPANDER, expanderNum,
					regNum + 6, configVal) != MV_OK) {
		mvOsPrintf("%s: Error: Enable Write to IO Expander at 0x%x failed\n", __func__
			   , mvBoardTwsiAddrGet(BOARD_DEV_TWSI_IO_EXPANDER, expanderNum));
		return MV_ERROR;
	}

	readVal &= ~(1 << offset);	 
	readVal |= (value << offset);

	if (mvBoardTwsiSet(BOARD_DEV_TWSI_IO_EXPANDER, expanderNum,
					regNum + 2, readVal) != MV_OK) {
		mvOsPrintf("%s: Error: Write to IO Expander at 0x%x failed\n", __func__
			   , mvBoardTwsiAddrGet(BOARD_DEV_TWSI_IO_EXPANDER, expanderNum));
		return MV_ERROR;
	}

	return MV_OK;
}
 
MV_32 mvBoardMppGet(MV_U32 mppGroupNum)
{
	return board->pBoardMppConfigValue->mppGroup[mppGroupNum];
}

MV_VOID mvBoardMppSet(MV_U32 mppGroupNum, MV_U32 mppValue)
{
	board->pBoardMppConfigValue->mppGroup[mppGroupNum] = mppValue;
}

MV_VOID mvBoardMppTypeSet(MV_U32 mppGroupNum, MV_U32 groupType)
{
	MV_U32 mppVal;
	MV_U32 mppGroups[MV_BOARD_MAX_MPP_GROUPS][MV_BOARD_MPP_GROUPS_MAX_TYPES] = MPP_GROUP_TYPES;

	mppVal = mppGroups[mppGroupNum][groupType];
	mvBoardMppSet(mppGroupNum,mppVal);

}

MV_VOID mvBoardInfoUpdate(MV_VOID)
{
	MV_U32 ethComplex;

	ethComplex = mvBoardEthComplexConfigGet();
	if (ethComplex & MV_ETHCOMP_GE_MAC0_2_GE_PHY_P0)
		mvBoardPhyAddrSet(0, 0x0);
	else if (ethComplex & MV_ETHCOMP_GE_MAC0_2_RGMII0)
		mvBoardPhyAddrSet(0, 0x0);
	else
		mvBoardPhyAddrSet(0, -1);  

	if (ethComplex & MV_ETHCOMP_GE_MAC1_2_GE_PHY_P3)
		mvBoardPhyAddrSet(1, 0x3);
	else if (ethComplex & MV_ETHCOMP_GE_MAC1_2_RGMII1)
		mvBoardPhyAddrSet(1, 0x1);
	else
		mvBoardPhyAddrSet(1, -1);  

	mvBoardMppIdUpdate();

}

MV_VOID mvBoardMppIdUpdate(MV_VOID)
{
	 
	mvBoardBootDeviceGroupSet();
}

MV_BOARD_BOOT_SRC mvBoardBootDeviceGroupSet()
{
	MV_BOARD_BOOT_SRC bootSrc = mvBoardBootDeviceGet();

	switch (bootSrc) {
	case MSAR_0_BOOT_NAND_NEW:
		mvBoardMppTypeSet(0, NAND_BOOT_V2);
		mvBoardMppTypeSet(1, NAND_BOOT_V2);
		break;
	case MSAR_0_BOOT_SPI_FLASH:
		mvBoardMppTypeSet(0, SPI0_BOOT);
		mvBoardMppTypeSet(1, SPI0_BOOT);
		break;
	default:
		return MV_ERROR;
	}
	return bootSrc;
}

MV_BOARD_BOOT_SRC mvBoardBootDeviceGet()
{
	MV_U32 satrBootDeviceValue = mvCtrlSatRRead(MV_SATR_BOOT_DEVICE);
	MV_SATR_BOOT_TABLE satrTable[] = MV_SATR_TABLE_VAL;
	MV_SATR_BOOT_TABLE satrBootEntry;
	MV_BOARD_BOOT_SRC defaultBootSrc;

#if defined(MV_SPI_BOOT)
	defaultBootSrc = MSAR_0_BOOT_SPI_FLASH;
	DB(mvOsPrintf("default boot source is SPI-0\n"));
#elif defined(MV_NAND_BOOT)
	defaultBootSrc = MSAR_0_BOOT_NAND_NEW;
	DB(mvOsPrintf("default boot source is NAND\n"));
#endif

	if (satrBootDeviceValue == MV_ERROR) {
		mvOsPrintf("%s: Error: failed to read boot source\n", __func__);
		mvOsPrintf("Using pre-compiled image type as boot source\n");
		return defaultBootSrc;
	}

	satrBootEntry = satrTable[satrBootDeviceValue];

	if (satrBootEntry.bootSrc == MSAR_0_BOOT_UART) {
		mvOsPrintf("\t** Booting from UART (restore DIP-switch to");
		mvOsPrintf("requested boot source before reset!) **\n");
		return defaultBootSrc;
	}

	if (satrBootEntry.bootSrc != MSAR_0_BOOT_SPI_FLASH)
		return satrBootEntry.bootSrc;

	if (mvBoardBootAttrGet(satrBootDeviceValue, 1) == MSAR_0_SPI0)
		return MSAR_0_BOOT_SPI_FLASH;
	else
		return MSAR_0_BOOT_SPI1_FLASH;
}

MV_U32 mvBoardBootAttrGet(MV_U32 satrBootDeviceValue, MV_U8 attrNum)
{
	MV_SATR_BOOT_TABLE satrTable[] = MV_SATR_TABLE_VAL;
	MV_SATR_BOOT_TABLE satrBootEntry = satrTable[satrBootDeviceValue];

	switch (attrNum) {
	case 1:
		return satrBootEntry.attr1;
		break;
	case 2:
		return satrBootEntry.attr2;
		break;
	case 3:
		return satrBootEntry.attr3;
		break;
	default:
		return MV_ERROR;
	}
}

MV_ETH_COMPLEX_TOPOLOGY mvBoardLaneSGMIIGet()
{
	MV_U32 laneConfig;
	 
	laneConfig = mvCtrlSysConfigGet(MV_CONFIG_LANE1);
	if (laneConfig == MV_ERROR)
		return MV_ERROR;
	else if (laneConfig == 0x1)
		return MV_ETHCOMP_GE_MAC0_2_COMPHY_1;
	 
	laneConfig = mvCtrlSysConfigGet(MV_CONFIG_LANE2);
	if (laneConfig == MV_ERROR)
		return MV_ERROR;
	else if (laneConfig == 0x0)
		return MV_ETHCOMP_GE_MAC0_2_COMPHY_2;
	 
	laneConfig = mvCtrlSysConfigGet(MV_CONFIG_LANE3);
	if (laneConfig == MV_ERROR)
		return MV_ERROR;
	else if (laneConfig == 0x1)
		return MV_ETHCOMP_GE_MAC0_2_COMPHY_3;

	mvOsPrintf("%s: Error: unexpected value for Serdes Lane board configuration\n", __func__);
	return MV_ERROR;
}

MV_STATUS mvBoardIsInternalSwitchConnectedToPort(MV_U32 ethPortNum)
{
	MV_U32 ethComplex = mvBoardEthComplexConfigGet();

	if (ethPortNum >= board->numBoardMacInfo) {
		mvOsPrintf("%s: Error: Illegal port number(%u)\n", __func__, ethPortNum);
		return MV_FALSE;
	}

	if ((ethPortNum == 0) && (ethComplex & MV_ETHCOMP_GE_MAC0_2_SW_P6))
		return MV_TRUE;
	else if ((ethPortNum == 1) && (ethComplex & MV_ETHCOMP_GE_MAC1_2_SW_P4))
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_STATUS mvBoardIsInternalSwitchConnected(void)
{
	MV_U32 ethComplex = mvBoardEthComplexConfigGet();

	if ((ethComplex & MV_ETHCOMP_GE_MAC0_2_SW_P6) ||
	    (ethComplex & MV_ETHCOMP_GE_MAC1_2_SW_P4))
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_32 mvBoardSwitchConnectedPortGet(MV_U32 ethPort)
{
	MV_U32 ethComplex = mvBoardEthComplexConfigGet();

	if (ethPort >= board->numBoardMacInfo) {
		mvOsPrintf("%s: Error: Illegal port number(%u)\n", __func__, ethPort);
		return MV_FALSE;
	}

	if ((ethPort == 0) && (ethComplex & MV_ETHCOMP_GE_MAC0_2_SW_P6))
		return 6;
	else if ((ethPort == 1) && (ethComplex & MV_ETHCOMP_GE_MAC1_2_SW_P4))
		return 4;
	else
		return -1;

}

MV_U32 mvBoardSwitchPortsMaskGet(MV_U32 switchIdx)
{
	MV_U32 mask = 0, c = mvBoardEthComplexConfigGet();

	if (c & MV_ETHCOMP_SW_P0_2_GE_PHY_P0)
		mask |= BIT0;
	if (c & MV_ETHCOMP_SW_P1_2_GE_PHY_P1)
		mask |= BIT1;
	if (c & MV_ETHCOMP_SW_P2_2_GE_PHY_P2)
		mask |= BIT2;
	if (c & MV_ETHCOMP_SW_P3_2_GE_PHY_P3)
		mask |= BIT3;
	if ((c & MV_ETHCOMP_SW_P4_2_RGMII0) || (c & MV_ETHCOMP_GE_MAC1_2_SW_P4))
		mask |= BIT4;
	if (c & MV_ETHCOMP_GE_MAC0_2_SW_P6)
		mask |= BIT6;

	return mask;
}

MV_U32 mvBoardSwitchPortForceLinkGet(MV_U32 switchIdx)
{
	return board->switchforceLinkMask;
}

MV_U32 mvBoardFreqModesNumGet()
{
	MV_U16 ctrlModel = mvCtrlModelGet();

	if (ctrlModel == MV_6720_DEV_ID)
		return FREQ_MODES_NUM_6720;

	mvOsPrintf("%s: Error: Illegal ctrl Model (%x)\n", __func__, ctrlModel);
	return MV_ERROR;
}

MV_VOID mvBoardConfigWrite(void)
{
	MV_U32 mppGroup, i, reg;
	MV_BOARD_SPEC_INIT *boardSpec;

	for (mppGroup = 0; mppGroup < MV_MPP_MAX_GROUP; mppGroup++) {
		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mvBoardMppGet(mppGroup));
	}

	boardSpec = mvBoardSpecInitGet();
	if (boardSpec != NULL) {
		i = 0;
		while (boardSpec[i].reg != TBL_TERM) {
			reg = MV_REG_READ(boardSpec[i].reg);
			reg &= ~boardSpec[i].mask;
			reg |= (boardSpec[i].val & boardSpec[i].mask);
			MV_REG_WRITE(boardSpec[i].reg, reg);
			i++;
		}
	}
}

MV_U32 mvBoardGppConfigGet(void)
{
	MV_U32 gpp, i, result = 0;

	for (i = 0; i < board->numBoardGppInfo; i++) {
		if (board->pBoardGppInfo[i].devClass == BOARD_GPP_CONF) {
			gpp = board->pBoardGppInfo[i].gppPinNum;
			result <<= 1;
			result |= (mvGppValueGet(gpp >> 5, 1 << (gpp & 0x1F)) >> (gpp & 0x1F));
		}
	}

	return result;
}

MV_32 mvBoardTdmSpiModeGet(MV_VOID)
{
	return 0;
}

MV_U8 mvBoardTdmDevicesCountGet(void)
{
	MV_16 index = board->boardTdmInfoIndex;

	if (index == -1)
		return 0;

	return board->numBoardTdmInfo[index];
}

MV_U8 mvBoardTdmSpiCsGet(MV_U8 devId)
{
	MV_16 index;

	index = board->boardTdmInfoIndex;
	if (index == -1)
		return 0;

	if (devId >= board->numBoardTdmInfo[index])
		return -1;

	return board->pBoardTdmInt2CsInfo[index][devId].spiCs;
}

MV_VOID mvBoardMppModuleTypePrint(MV_VOID)
{
	MV_U32 i, ethConfig = mvBoardEthComplexConfigGet();

	mvOsOutput("Board configuration detected:\n");

	if (ethConfig & MV_ETHCOMP_GE_MAC0_2_RGMII0)
		mvOsOutput("\tRGMII0 Module on MAC0\n");
	if (ethConfig & MV_ETHCOMP_GE_MAC1_2_RGMII1)
		mvOsOutput("\tRGMII1 on MAC1\n");
	if (ethConfig & MV_ETHCOMP_SW_P4_2_RGMII0)
		mvOsOutput("\tRGMII0 Module on Switch port #4\n");

	if (ethConfig & MV_ETHCOMP_GE_MAC0_2_GE_PHY_P0)
			mvOsOutput("\tGE-PHY-0 on MAC0\n");
	if (ethConfig & MV_ETHCOMP_GE_MAC1_2_GE_PHY_P3)
			mvOsOutput("\tGE-PHY-3 on MAC1\n");
	if ((ethConfig & MV_ETHCOMP_SW_P0_2_GE_PHY_P0) && (ethConfig & MV_ETHCOMP_SW_P1_2_GE_PHY_P1)
		&& (ethConfig & MV_ETHCOMP_SW_P2_2_GE_PHY_P2) && (ethConfig & MV_ETHCOMP_SW_P3_2_GE_PHY_P3))
			mvOsOutput("\t4xGE-PHY Module on 4 Switch ports\n");
	else {
		if (ethConfig & MV_ETHCOMP_SW_P0_2_GE_PHY_P0)
			mvOsOutput("\tGE-PHY-0 Module on Switch port #0\n");
		if (ethConfig & MV_ETHCOMP_SW_P1_2_GE_PHY_P1)
			mvOsOutput("\tGE-PHY-1 Module on Switch port #1\n");
		if (ethConfig & MV_ETHCOMP_SW_P2_2_GE_PHY_P2)
			mvOsOutput("\tGE-PHY-2 Module on Switch port #2\n");
		if (ethConfig & MV_ETHCOMP_SW_P3_2_GE_PHY_P3)
			mvOsOutput("\tGE-PHY-3 Module on Switch port #3\n");
	}
	if (ethConfig & MV_ETHCOMP_GE_MAC1_2_PON_ETH_SERDES_SFP)
		mvOsOutput("\tPON ETH SERDES on MAC1 [SFP]\n");

	mvOsOutput("SERDES configuration:\n");
	for (i = 0; i < 4; i++) {
		switch (mvCtrlLaneSelectorGet(i)) {
		case PEX_UNIT_ID:
			mvOsOutput("\tLane #%d: PCIe%d\n", i, i);
			break;
		case USB3_UNIT_ID:
			mvOsOutput("\tLane #%d: USB3\n", i);
			break;
		case SATA_UNIT_ID:
			mvOsOutput("\tLane #%d: SATA%d\n", i, (i == 1 ? 1 : 0));
			break;
		case SGMII_UNIT_ID:
			mvOsOutput("\tLane #%d: SGMII\n", i);
			break;
		default:
			break;
		}
	}
}

MV_VOID mvBoardOtherModuleTypePrint(MV_VOID)
{
	 
	if (mvBoardIsPexModuleConnected())
		mvOsOutput("       PEX module.\n");
	 
	if (mvBoardIsSetmModuleConnected())
		mvOsOutput("       SETM module.\n");
	 
	if (mvBoardIsLvdsModuleConnected())
		mvOsOutput("       LVDS module.\n");
}

MV_BOOL mvBoardIsGbEPortConnected(MV_U32 ethPortNum)
{
	return MV_FALSE;
}

MV_32 mvBoardGetDevicesNumber(MV_BOARD_DEV_CLASS devClass)
{
	MV_U32 foundIndex = 0, devNum;

	for (devNum = START_DEV_CS; devNum < board->numBoardDeviceIf; devNum++)
		if (board->pDevCsInfo[devNum].devClass == devClass)
			foundIndex++;

	return foundIndex;
}

MV_32 mvBoardGetDeviceBaseAddr(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry = boardGetDevEntry(devNum, devClass);

	if (devEntry)
		return mvCpuIfTargetWinBaseLowGet(DEV_TO_TARGET(devEntry->deviceCS));

	return 0xFFFFFFFF;
}

MV_32 mvBoardGetDeviceBusWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry = boardGetDevEntry(devNum, devClass);

	if (devEntry)
		return devEntry->busWidth;

	return 0xFFFFFFFF;
}

MV_32 mvBoardGetDeviceWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry = boardGetDevEntry(devNum, devClass);

	if (devEntry)
		return devEntry->devWidth;

	return MV_ERROR;
}

MV_32 mvBoardGetDeviceWinSize(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry = boardGetDevEntry(devNum, devClass);

	if (devEntry)
		return mvCpuIfTargetWinSizeGet(DEV_TO_TARGET(devEntry->deviceCS));

	return 0xFFFFFFFF;
}

static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_U32 foundIndex = 0, devIndex;

	for (devIndex = START_DEV_CS; devIndex < board->numBoardDeviceIf; devIndex++) {
		if (board->pDevCsInfo[devIndex].devClass == devClass) {
			if (foundIndex == devNum)
				return &(board->pDevCsInfo[devIndex]);
			foundIndex++;
		}
	}

	return NULL;
}

MV_U32 boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry = boardGetDevEntry(devNum, devClass);

	if (devEntry)
		return devEntry->deviceCS;

	return 0xFFFFFFFF;
}
 
MV_STATUS mvBoardSgmiiSfp1TxSet(MV_BOOL enable)
{
	return mvBoardIoExpValSet(0, 0, 1 , (enable ? 0x0 : 0x1));
}

MV_U8 mvBoardTwsiAddrTypeGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;

	for (i = 0; i < board->numBoardTwsiDev; i++) {
		if (board->pBoardTwsiDev[i].devClass == twsiClass) {
			if (indexFound == index)
				return board->pBoardTwsiDev[i].twsiDevAddrType;
			else
				indexFound++;
		}
	}
	DB(mvOsPrintf("%s: Error: read TWSI address type failed\n", __func__));
	return MV_ERROR;
}

MV_U8 mvBoardTwsiAddrGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index)
{
	int i;

	for (i = 0; i < board->numBoardTwsiDev; i++) {
		if ((board->pBoardTwsiDev[i].devClass == twsiClass) \
				&& (board->pBoardTwsiDev[i].devClassId == index)){
			return board->pBoardTwsiDev[i].twsiDevAddr;
		}
	}

	return 0xFF;
}

MV_U32 mvBoardEthComplexConfigGet(MV_VOID)
{
	return board->pBoardModTypeValue->ethSataComplexOpt;
}

MV_VOID mvBoardEthComplexConfigSet(MV_U32 ethConfig)
{
	 
	board->pBoardModTypeValue->ethSataComplexOpt = ethConfig;
	return;
}

MV_STATUS mvBoardSatrInfoConfig(MV_SATR_TYPE_ID satrClass, MV_BOARD_SATR_INFO *satrInfo, MV_BOOL read)
{
	int i, start, end;
	MV_U32 boardId = mvBoardIdIndexGet(mvBoardIdGet());

	if (read == MV_TRUE) {	 
		start = 0;
		end = MV_SATR_READ_MAX_OPTION;
	} else {		 
		start = MV_SATR_READ_MAX_OPTION;
		end = MV_SATR_WRITE_MAX_OPTION;
	}

	for (i = start; i < end ; i++)
		if (boardSatrInfo[i].satrId == satrClass) {
			*satrInfo = boardSatrInfo[i];
			 
			if (read == MV_TRUE || boardSatrInfo[i].isActiveForBoard[boardId])
				return MV_OK;
			else
				return MV_ERROR;
		}
	DB(mvOsPrintf("%s: Error: requested MV_SATR_TYPE_ID was not found (%d)\n", __func__,satrClass));
	return MV_ERROR;
}

MV_BOOL mvBoardConfigTypeGet(MV_CONFIG_TYPE_ID configClass, MV_BOARD_CONFIG_TYPE_INFO *configInfo)
{
	int i;
	MV_U32 boardId = mvBoardIdIndexGet(mvBoardIdGet());

	for (i = 0; i < MV_CONFIG_TYPE_MAX_OPTION ; i++)
		if (boardConfigTypesInfo[i].configId == configClass) {
			*configInfo = boardConfigTypesInfo[i];
			if (boardConfigTypesInfo[i].isActiveForBoard[boardId])
				return MV_TRUE;
			else
				return MV_FALSE;
		}
	mvOsPrintf("%s: Error: requested MV_CONFIG_TYPE_ID was not found (%d)\n", __func__, configClass);
	return MV_FALSE;
}

MV_STATUS mvBoardExtPhyBufferSelect(MV_BOOL enable)
{
	return MV_FALSE;
}

MV_32 mvBoardNandWidthGet(void)
{
	MV_U32 devNum;
	MV_U32 devWidth;

	for (devNum = START_DEV_CS; devNum < board->numBoardDeviceIf; devNum++) {
		devWidth = mvBoardGetDeviceWidth(devNum, BOARD_DEV_NAND_FLASH);
		if (devWidth != MV_ERROR)
			return devWidth / 8;
	}

	DB(mvOsPrintf("%s: Error: NAND device was not found\n", __func__));
	return MV_ERROR;
}

static MV_U32 gBoardId = -1;
MV_VOID mvBoardSet(MV_U32 boardId)
{
	 
	if (boardId >= MARVELL_BOARD_ID_BASE && boardId < MV_MAX_MARVELL_BOARD_ID) {  
		board = marvellBoardInfoTbl[mvBoardIdIndexGet(boardId)];
		gBoardId = boardId;
	} else if (boardId >= CUTOMER_BOARD_ID_BASE && boardId < MV_MAX_CUSTOMER_BOARD_ID) {  
		board = customerBoardInfoTbl[mvBoardIdIndexGet(boardId)];
		gBoardId = boardId;
	} else
		mvOsPrintf("%s: Error: wrong boardId (%d)\n", __func__, boardId);
}

MV_U32 mvBoardIdGet(MV_VOID)
{
	if (gBoardId != -1)
		return gBoardId;

#ifdef CONFIG_CUSTOMER_BOARD_SUPPORT
	#ifdef CONFIG_CUSTOMER_BOARD_0
		gBoardId = ARMADA_375_CUSTOMER_BOARD_ID0;
	#elif CONFIG_CUSTOMER_BOARD_1
		gBoardId = ARMADA_375_CUSTOMER_BOARD_ID1;
	#endif
#else

#if 0
	MV_U32 readValue;

	readValue = MV_REG_READ(MPP_SAMPLE_AT_RESET(1));
	readValue = ((readValue & (0xF0)) >> 4);

	if (readValue == DB_6720_HW_ID)
		readValue = 0x0;

	if (readValue < MV_MARVELL_BOARD_NUM && readValue >= 0) {
		gBoardId = MARVELL_BOARD_ID_BASE + readValue;
	} else {
		mvOsPrintf("%s: Error: read wrong board (%d)\n", __func__, readValue);
		return MV_INVALID_BOARD_ID;
	}
#endif  

	gBoardId = DB_6720_ID;
#endif

	return gBoardId;
}

MV_STATUS mvBoardTwsiGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U8 devNum, MV_U8 regNum, MV_U8 *pData)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	MV_U8 data, chanNum = 0;

	slave.type = ADDR7_BIT;
	slave.address = 0;

	DB(mvOsPrintf("Board: TWSI Read device\n"));
	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(twsiClass, devNum);
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(twsiClass, devNum);

	if (twsiClass == BOARD_DEV_TWSI_IO_EXPANDER)
		chanNum = 1;

	mvTwsiInit(chanNum, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	twsiSlave.validOffset = MV_TRUE;
	 
	twsiSlave.offset = regNum;
	twsiSlave.moreThen256 = MV_FALSE;

	if (MV_OK != mvTwsiRead(chanNum, &twsiSlave, &data, 1)) {
		mvOsPrintf("%s: Twsi Read fail\n", __func__);
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Read S@R succeded\n"));

	*pData = data;
	return MV_OK;
}

MV_STATUS mvBoardTwsiSet(MV_BOARD_TWSI_CLASS twsiClass, MV_U8 devNum, MV_U8 regNum, MV_U8 regVal)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	MV_U8 chanNum = 0;

	slave.type = ADDR7_BIT;
	slave.address = 0;

	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(twsiClass, devNum);
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(twsiClass, devNum);
	twsiSlave.validOffset = MV_TRUE;

	if (twsiClass == BOARD_DEV_TWSI_IO_EXPANDER)
		chanNum = 1;

	mvTwsiInit(chanNum, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	DB(mvOsPrintf("%s: TWSI Write addr %x, type %x, data %x\n", __func__,
		      twsiSlave.slaveAddr.address, twsiSlave.slaveAddr.type, regVal));
	 
	twsiSlave.offset = regNum;
	twsiSlave.moreThen256 = MV_FALSE;
	if (MV_OK != mvTwsiWrite(chanNum, &twsiSlave, &regVal, 1)) {
		DB(mvOsPrintf("%s: Write S@R fail\n", __func__));
		return MV_ERROR;
	}
	DB(mvOsPrintf("%s: Write S@R succeded\n", __func__));

	return MV_OK;
}

MV_U8 mvBoardCpuCoresNumGet(MV_VOID)
{
	return 1;
}

MV_STATUS mvBoardMppModulesScan(void)
{
	return MV_OK;
}

MV_BOOL mvBoardIsPexModuleConnected(void)
{
	return MV_FALSE;
}

MV_BOOL mvBoardIsSetmModuleConnected(void)
{
	return MV_FALSE;
}

MV_BOOL mvBoardIsLvdsModuleConnected(void)
{
	return MV_FALSE;
}

MV_BOOL mvBoardIsLcdDviModuleConnected(void)
{
	return MV_FALSE;
}

MV_STATUS mvBoardTwsiMuxChannelSet(MV_U8 muxChNum)
{
	return MV_ERROR;
}

MV_STATUS mvBoardTwsiReadByteThruMux(MV_U8 muxChNum, MV_U8 chNum,
				     MV_TWSI_SLAVE *pTwsiSlave, MV_U8 *data)
{
	return MV_ERROR;
}

MV_32 mvBoardSmiScanModeGet(MV_U32 switchIdx)
{
	return BOARD_ETH_SWITCH_SMI_SCAN_MODE;
}

MV_U32 mvBoardSwitchCpuPortGet(MV_U32 switchIdx)
{
	MV_U32 c = board->pBoardModTypeValue->ethSataComplexOpt;
	MV_U32 cpuPort = -1;

	if (c & MV_ETHCOMP_GE_MAC0_2_SW_P6)
		cpuPort = 6;
	else if (c & MV_ETHCOMP_GE_MAC1_2_SW_P4)
		cpuPort = 4;
	else
		mvOsPrintf("%s: Error: No CPU port.\n", __func__);

	return cpuPort;
}

MV_BOOL mvBoardIsEthConnected(MV_U32 ethNum)
{
	MV_U32 c = mvBoardEthComplexConfigGet();
	MV_BOOL isActive = MV_FALSE;

	if (ethNum == 0 && ((c & MV_ETHCOMP_GE_MAC0_2_GE_PHY_P0) ||
			(c & MV_ETHCOMP_GE_MAC0_2_RGMII0)))
			isActive = MV_TRUE;

	if (ethNum == 1 && ((c & MV_ETHCOMP_GE_MAC1_2_GE_PHY_P3) ||
			    (c & MV_ETHCOMP_GE_MAC1_2_RGMII1) ||
			    (c & MV_ETHCOMP_GE_MAC1_2_PON_ETH_SERDES_SFP)))
			isActive = MV_TRUE;

	return isActive;
}

MV_BOOL mvBoardIsEthActive(MV_U32 ethNum)
{
	 
	return mvBoardIsEthConnected(ethNum);
}

MV_BOOL mvBoardIsQsgmiiModuleConnected(MV_VOID)
{
	return MV_FALSE;
}

MV_32 mvBoardGePhySwitchPortGet(MV_VOID)
{
	return -1;
}

MV_32 mvBoardRgmiiASwitchPortGet(MV_VOID)
{
	return -1;
}

MV_32 mvBoardSwitchPortMap(MV_U32 switchIdx, MV_U32 switchPortNum)
{
	MV_U32 ethComplex = mvBoardEthComplexConfigGet();
	if (switchPortNum >= BOARD_ETH_SWITCH_PORT_NUM) {
		mvOsPrintf("%s: Error: wrong switch port number (%d)\n", __func__, switchPortNum);
		return -1;
	}

	if ((switchPortNum == 0) && (ethComplex & MV_ETHCOMP_SW_P0_2_GE_PHY_P0))
		return 0;
	else if ((switchPortNum == 1) && (ethComplex & MV_ETHCOMP_SW_P1_2_GE_PHY_P1))
		return 1;
	else if ((switchPortNum == 2) && (ethComplex & MV_ETHCOMP_SW_P2_2_GE_PHY_P2))
		return 2;
	else if ((switchPortNum == 3) && (ethComplex & MV_ETHCOMP_SW_P3_2_GE_PHY_P3))
		return 3;
	else if ((switchPortNum == 4) && (ethComplex & MV_ETHCOMP_SW_P4_2_RGMII0))
		return 4;

	mvOsPrintf("%s: Error: switch port map not found\n", __func__);
	return -1;
}

MV_BOARD_PEX_INFO *mvBoardPexInfoGet(void)
{
	return &board->boardPexInfo;
}

MV_BOOL mvBoardConfigAutoDetectEnabled()
{
	return board->configAutoDetect;
}

MV_NFC_ECC_MODE mvBoardNandECCModeGet()
{
#if defined(MV_NAND_4BIT_MODE)
	return MV_NFC_ECC_BCH_2K;
#elif defined(MV_NAND_8BIT_MODE)
	return MV_NFC_ECC_BCH_1K;
#elif defined(MV_NAND_12BIT_MODE)
	return MV_NFC_ECC_BCH_704B;
#elif defined(MV_NAND_16BIT_MODE)
	return MV_NFC_ECC_BCH_512B;
#else
	MV_U32 satrBootDeviceValue;
	MV_SATR_BOOT_TABLE satrTable[] = MV_SATR_TABLE_VAL;

	if (mvBoardBootDeviceGet() == MSAR_0_BOOT_NAND_NEW) {
		satrBootDeviceValue = mvCtrlSatRRead(MV_SATR_BOOT_DEVICE);
		switch (satrTable[satrBootDeviceValue].attr3) {
		case MSAR_0_NAND_ECC_4BIT:
			return MV_NFC_ECC_BCH_2K;
		case MSAR_0_NAND_ECC_8BIT:
			return MV_NFC_ECC_BCH_1K;
		case MSAR_0_NAND_ECC_12BIT:
			return MV_NFC_ECC_BCH_704B;
		case MSAR_0_NAND_ECC_16BIT:
			return MV_NFC_ECC_BCH_512B;
		default:
			break;
		}
	}
	return MV_NFC_ECC_DISABLE;
#endif
}
