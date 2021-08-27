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
#include "device/mvDevice.h"
#include "neta/gbe/mvEthRegs.h"
#include "gpp/mvGppRegs.h"

#undef MV_DEBUG
#ifdef MV_DEBUG
#define DB(x)	x
#define DB1(x)	x
#else
#define DB(x)
#define DB1(x)
#endif

#define CODE_IN_ROM		MV_FALSE
#define CODE_IN_RAM		MV_TRUE

extern MV_BOARD_INFO *boardInfoTbl[];
#define BOARD_INFO(boardId)	boardInfoTbl[boardId - BOARD_ID_BASE]

static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);

MV_U32 tClkRate = -1;
extern MV_U8 mvDbDisableModuleDetection;

MV_U32 gSerdesZ1AMode = 0;

MV_VOID mvBoardEnvInit(MV_VOID)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_U32 nandDev;
	MV_U32 norDev;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardEnvInit:Board unknown.\n");
		return;
	}

#ifdef MY_DEF_HERE
	 
#else
	nandDev = boardGetDevCSNum(0, BOARD_DEV_NAND_FLASH);
	if (nandDev != 0xFFFFFFFF) {
		 
		nandDev = BOOT_CS;
		MV_REG_WRITE(DEV_BANK_PARAM_REG(nandDev), BOARD_INFO(boardId)->nandFlashReadParams);
		MV_REG_WRITE(DEV_BANK_PARAM_REG_WR(nandDev), BOARD_INFO(boardId)->nandFlashWriteParams);
		MV_REG_WRITE(DEV_NAND_CTRL_REG, BOARD_INFO(boardId)->nandFlashControl);
	}

	norDev = boardGetDevCSNum(0, BOARD_DEV_NOR_FLASH);
	if (norDev != 0xFFFFFFFF) {
		 
		MV_REG_WRITE(DEV_BANK_PARAM_REG(norDev), BOARD_INFO(boardId)->norFlashReadParams);
		MV_REG_WRITE(DEV_BANK_PARAM_REG_WR(norDev), BOARD_INFO(boardId)->norFlashWriteParams);
		MV_REG_WRITE(DEV_BUS_SYNC_CTRL, 0x11);
	}

	MV_REG_WRITE(GPP_DATA_OUT_REG(0), BOARD_INFO(boardId)->gppOutValLow);
	MV_REG_WRITE(GPP_DATA_OUT_REG(1), BOARD_INFO(boardId)->gppOutValMid);
	MV_REG_WRITE(GPP_DATA_OUT_REG(2), BOARD_INFO(boardId)->gppOutValHigh);

	mvGppPolaritySet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValLow);
	mvGppPolaritySet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValMid);
	mvGppPolaritySet(2, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValHigh);

	mvGppTypeSet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValLow);
	mvGppTypeSet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValMid);
	mvGppTypeSet(2, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValHigh);
#endif
}
 
MV_U16 mvBoardModelGet(MV_VOID)
{
	return (mvBoardIdGet() >> 16);
}
 
MV_U16 mvBoardRevGet(MV_VOID)
{
	return (mvBoardIdGet() & 0xFFFF);
}
 
MV_STATUS mvBoardNameGet(char *pNameBuff)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsSPrintf(pNameBuff, "Board unknown.\n");
		return MV_ERROR;
	}
	if (mvCtrlModelRevGet() ==  MV_6710_Z1_ID)
		mvOsSPrintf(pNameBuff, "%s", "DB-6710-Z1");
	else
		mvOsSPrintf(pNameBuff, "%s", BOARD_INFO(boardId)->boardName);

	return MV_OK;
}
 
MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	switch (boardId) {
	case DB_88F78XX0_BP_REV2_ID:
	case DB_88F78XX0_BP_ID:   
		if (ethPortNum > 1)
			return MV_TRUE;
		break;
	case DB_78X60_AMC_ID:
		if (ethPortNum > 0)
			return MV_TRUE;
		break;
	case RD_78460_SERVER_ID:
	case RD_78460_SERVER_REV2_ID:
		if (ethPortNum > 0)
			return MV_TRUE;
		break;
	case DB_78X60_PCAC_ID:
	case DB_784MP_GP_ID:
	case RD_78460_NAS_ID:
	case RD_78460_CUSTOMER_ID:
	case DB_78X60_PCAC_REV2_ID:
		return MV_TRUE;
		break;
#ifdef MY_DEF_HERE
	case SYNO_AXP_4BAY_2BAY:
	case SYNO_AXP_4BAY_RACK:
		return MV_FALSE;
		break;
	case SYNO_AXP_2BAY:
		return MV_TRUE;
		break;
#endif

	default:
		DB(mvOsPrintf("mvBoardSerdesCfgGet: Unsupported board!\n"));
		return MV_FALSE;
	}

	return MV_FALSE;
}
 
MV_BOOL mvBoardIsPortInGmii(MV_U32 ethPortNum)
{
	if (mvBoardIsGMIIModuleConnected() && (ethPortNum ==0))
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_32 mvBoardSwitchCpuPortGet(MV_U32 switchIdx)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchCpuPortGet: Board unknown.\n");
		return -1;
	}
	if ((BOARD_INFO(boardId)->switchInfoNum == 0) || (switchIdx >= BOARD_INFO(boardId)->switchInfoNum))
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[switchIdx].cpuPort;
}

MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardPhyAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr;
}
 
MV_32 mvBoardQuadPhyAddr0Get(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardQuadPhyAddr0Get: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr0;
}

MV_32 mvBoardPhyLinkCryptPortAddrGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardPhyLinkCryptPortAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].LinkCryptPortAddr;
}

MV_BOOL mvBoardIsPortInRgmii(MV_U32 ethPortNum)
{
	return !mvBoardIsPortInGmii(ethPortNum);
}

MV_BOARD_MAC_SPEED mvBoardMacSpeedGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardMacSpeedGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (boardId == RD_78460_NAS_ID) {
		if (mvBoardIsSwitchModuleConnected())
			return BOARD_MAC_SPEED_1000M;
	}
	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardMacSpeed;
}

MV_BOOL mvBoardSpecInitGet(MV_U32 *regOff, MV_U32 *data)
{
	return MV_FALSE;
}

MV_U32 mvBoardTclkGet(MV_VOID)
{
	if (mvBoardIdGet() == FPGA_88F78XX0_ID)
		return MV_FPGA_CLK;  

	if ((MV_REG_READ(MPP_SAMPLE_AT_RESET(0)) & MSAR_TCLK_MASK) != 0)
		return MV_BOARD_TCLK_200MHZ;
	else
		return MV_BOARD_TCLK_250MHZ;
}

MV_U32 mvBoardSysClkGet(MV_VOID)
{
	MV_U32 idx;
	MV_U32 cpuFreqMhz, ddrFreqMhz;
	MV_CPU_ARM_CLK_RATIO clockRatioTbl[] = MV_DDR_L2_CLK_RATIO_TBL;

	if (mvBoardIdGet() == FPGA_88F78XX0_ID)
		return MV_FPGA_CLK;  

	idx = MSAR_DDR_L2_CLK_RATIO_IDX(MV_REG_READ(MPP_SAMPLE_AT_RESET(0)),
									MV_REG_READ(MPP_SAMPLE_AT_RESET(1)));

	if (clockRatioTbl[idx].vco2cpu != 0) {	 
		cpuFreqMhz = mvCpuPclkGet() / 1000000;	 
		cpuFreqMhz *= clockRatioTbl[idx].vco2cpu;	 
		ddrFreqMhz = cpuFreqMhz / clockRatioTbl[idx].vco2ddr;
		 
		if (((cpuFreqMhz % clockRatioTbl[idx].vco2ddr) * 10 / clockRatioTbl[idx].vco2ddr) >= 5)
			ddrFreqMhz++;

		return ddrFreqMhz * 1000000;
	} else
		return 0;
}

MV_U32 mvBoardDebugLedNumGet(MV_U32 boardId)
{
	return BOARD_INFO(boardId)->activeLedsNumber;
}

MV_VOID mvBoardDebugLed(MV_U32 hexNum)
{
	MV_U32 val[MV_GPP_MAX_GROUP] = {0};
	MV_U32 mask[MV_GPP_MAX_GROUP] = {0};
	MV_U32 digitMask;
	MV_U32 i, pinNum, gppGroup;
	MV_U32 boardId = mvBoardIdGet();

	if (BOARD_INFO(boardId)->pLedGppPin == NULL)
		return;

	hexNum &= (1 << BOARD_INFO(boardId)->activeLedsNumber) - 1;

	for (i = 0, digitMask = 1; i < BOARD_INFO(boardId)->activeLedsNumber; i++, digitMask <<= 1) {
			pinNum = BOARD_INFO(boardId)->pLedGppPin[i];
			gppGroup = pinNum / 32;
			if (hexNum & digitMask)
				val[gppGroup]  |= (1 << (pinNum - gppGroup * 32));
			mask[gppGroup] |= (1 << (pinNum - gppGroup * 32));
	}

	for (gppGroup = 0; gppGroup < MV_GPP_MAX_GROUP; gppGroup++) {
		 
		if (mask[gppGroup])
			mvGppValueSet(gppGroup, mask[gppGroup], BOARD_INFO(boardId)->ledsPolarity == 0 ?
					val[gppGroup] : ~val[gppGroup]);
	}
}

MV_32 mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS gppClass, MV_U32 index)
{
	MV_U32 boardId, i;
	MV_U32 indexFound = 0;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardRTCGpioPinGet:Board unknown.\n");
		return MV_ERROR;
	}

	for (i = 0; i < BOARD_INFO(boardId)->numBoardGppInfo; i++) {
		if (BOARD_INFO(boardId)->pBoardGppInfo[i].devClass == gppClass) {
			if (indexFound == index)
				return (MV_U32) BOARD_INFO(boardId)->pBoardGppInfo[i].gppPinNum;
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
	if ((type != BOARD_GPP_SDIO_POWER) && (type != BOARD_GPP_SDIO_DETECT) && (type != BOARD_GPP_SDIO_WP))
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
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGpioIntMaskGet:Board unknown.\n");
		return MV_ERROR;
	}

	switch (gppGrp) {
	case (0):
		return BOARD_INFO(boardId)->intsGppMaskLow;
		break;
	case (1):
		return BOARD_INFO(boardId)->intsGppMaskMid;
		break;
	case (2):
		return BOARD_INFO(boardId)->intsGppMaskHigh;
		break;
	default:
		return MV_ERROR;
	}
}

MV_32 mvBoardMppGet(MV_U32 mppGroupNum)
{
	MV_U32 boardId;
	MV_U32 mppMod;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
		return MV_ERROR;
	}

	mppMod = BOARD_INFO(boardId)->pBoardModTypeValue->boardMppMod;
	if (mppMod >= BOARD_INFO(boardId)->numBoardMppConfigValue)
		mppMod = 0;  

	return BOARD_INFO(boardId)->pBoardMppConfigValue[mppMod].mppGroup[mppGroupNum];
}

MV_U32 mvBoardGppConfigGet(void)
{
	MV_U32 boardId, i;
	MV_U32 result = 0;
	MV_U32 gpp;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGppConfigGet: Board unknown.\n");
		return 0;
	}

	for (i = 0; i < BOARD_INFO(boardId)->numBoardGppInfo; i++) {
		if (BOARD_INFO(boardId)->pBoardGppInfo[i].devClass == BOARD_GPP_CONF) {
			gpp = BOARD_INFO(boardId)->pBoardGppInfo[i].gppPinNum;
			result <<= 1;
			result |= (mvGppValueGet(gpp >> 5, 1 << (gpp & 0x1F)) >> (gpp & 0x1F));
		}
	}
	return result;

}

MV_32 mvBoardTdmSpiModeGet(MV_VOID)
{
	return DUAL_CHIP_SELECT_MODE;
}

MV_U8 mvBoardTdmDevicesCountGet(void)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_16 index;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardTdmDevicesCountGet: Board unknown.\n");
		return 0;
	}

	index = BOARD_INFO(boardId)->boardTdmInfoIndex;
	if (index == -1)
		return 0;

	return BOARD_INFO(boardId)->numBoardTdmInfo[index];
}

MV_U8 mvBoardTdmSpiCsGet(MV_U8 devId)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_16 index;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardTdmDevicesCountGet: Board unknown.\n");
		return -1;
	}

	index = BOARD_INFO(boardId)->boardTdmInfoIndex;
	if (index == -1)
		return 0;

	if (devId >= BOARD_INFO(boardId)->numBoardTdmInfo[index])
		return -1;

	return BOARD_INFO(boardId)->pBoardTdmInt2CsInfo[index][devId].spiCs;
}

MV_VOID mvBoardMppModuleTypePrint(MV_VOID)
{
	mvOsOutput("Modules Detected:\n");

	if (mvBoardTdmDevicesCountGet() > 0)
		mvOsOutput("       TDM module.\n");

	if (mvBoardIsLcdDviModuleConnected())
		mvOsOutput("       LCD DVI module.\n");

	if (mvBoardIsSwitchModuleConnected())
		mvOsOutput("       Switch module.\n");

	if (mvBoardIsGMIIModuleConnected())
		mvOsOutput("       GMII module.\n");

	return;
}

MV_VOID mvBoardOtherModuleTypePrint(MV_VOID)
{
	 
	if (mvBoardIsPexModuleConnected())
		mvOsOutput("       PEX module.\n");
	 
	if (mvBoardIsSetmModuleConnected())
		mvOsOutput("       SETM module.\n");
	 
	if (mvBoardIsLvdsModuleConnected())
		mvOsOutput("       LVDS module.\n");

	return;
}

MV_BOOL mvBoardIsGbEPortConnected(MV_U32 ethPortNum)
{
#ifdef MY_DEF_HERE
	MV_U32 boardId = mvBoardIdGet();
	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsGbEPortConnected: Board unknown.\n");
		return MV_FALSE;
	}
	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
		return MV_FALSE;
#endif
	switch (ethPortNum) {
	case 0:
		if (mvBoardIsLcdDviModuleConnected())
			return MV_FALSE;
		break;
	case 1:
		if (mvBoardIsLcdDviModuleConnected())
			return MV_FALSE;
		else if (mvBoardIsGMIIModuleConnected())
			return MV_FALSE;
		break;
	case 2:
		if ( (mvBoardIsPexModuleConnected()) || (mvBoardIsSetmModuleConnected()) )
			return MV_FALSE;
		break;
	case 3:
		break;
	default:
		break;
	}

	return MV_TRUE;
}

MV_32 mvBoardGetDevicesNumber(MV_BOARD_DEV_CLASS devClass)
{
	MV_U32 foundIndex = 0, devNum;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGetDeviceNumber:Board unknown.\n");
		return 0xFFFFFFFF;
	}

	for (devNum = START_DEV_CS; devNum < BOARD_INFO(boardId)->numBoardDeviceIf; devNum++) {
		if (BOARD_INFO(boardId)->pDevCsInfo[devNum].devClass == devClass)
			foundIndex++;
	}

	return foundIndex;
}

MV_32 mvBoardGetDeviceBaseAddr(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return mvCpuIfTargetWinBaseLowGet(DEV_TO_TARGET(devEntry->deviceCS));

	return 0xFFFFFFFF;
}

MV_32 mvBoardGetDeviceBusWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->busWidth;

	return 0xFFFFFFFF;
}

MV_32 mvBoardGetDeviceWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->devWidth;

	return MV_ERROR;
}

MV_32 mvBoardGetDeviceWinSize(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return mvCpuIfTargetWinSizeGet(DEV_TO_TARGET(devEntry->deviceCS));

	return 0xFFFFFFFF;
}

static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_U32 foundIndex = 0, devIndex;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("boardGetDevEntry: Board unknown.\n");
		return NULL;
	}

	for (devIndex = START_DEV_CS; devIndex < BOARD_INFO(boardId)->numBoardDeviceIf; devIndex++) {
		if (BOARD_INFO(boardId)->pDevCsInfo[devIndex].devClass == devClass) {
			if (foundIndex == devNum)
				return &(BOARD_INFO(boardId)->pDevCsInfo[devIndex]);
			foundIndex++;
		}
	}

	return NULL;
}

MV_U32 boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->deviceCS;

	return 0xFFFFFFFF;
}

MV_U8 mvBoardTwsiAddrTypeGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId = mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++) {
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == twsiClass) {
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
			else
				indexFound++;
		}
	}
	return (MV_ERROR);
}

MV_U8 mvBoardTwsiAddrGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId = mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++) {
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == twsiClass) {
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
			else
				indexFound++;
		}
	}
	return (0xFF);
}

MV_32 mvBoardNandWidthGet(void)
{
	MV_U32 devNum;
	MV_U32 devWidth;
	MV_U32 boardId = mvBoardIdGet();

	for (devNum = START_DEV_CS; devNum < BOARD_INFO(boardId)->numBoardDeviceIf; devNum++) {
		devWidth = mvBoardGetDeviceWidth(devNum, BOARD_DEV_NAND_FLASH);
		if (devWidth != MV_ERROR)
			return (devWidth / 8);
	}

	return MV_ERROR;
}

MV_U32 gBoardId = -1;
 
MV_VOID mvBoardIdSet(MV_VOID)
{
	if (gBoardId == -1) {
#if defined(DB_88F78X60)
		gBoardId = DB_88F78XX0_BP_ID;
#elif defined(RD_88F78460_SERVER)
		gBoardId = RD_78460_SERVER_ID;
#elif defined(RD_78460_SERVER_REV2)
		gBoardId = RD_78460_SERVER_REV2_ID;
#elif defined(DB_78X60_PCAC)
		gBoardId = DB_78X60_PCAC_ID;
#elif defined(DB_88F78X60_REV2)
		gBoardId = DB_88F78XX0_BP_REV2_ID;
#elif defined(RD_78460_NAS)
		gBoardId = RD_78460_NAS_ID;
#elif defined(DB_78X60_AMC)
		gBoardId = DB_78X60_AMC_ID;
#elif defined(DB_78X60_PCAC_REV2)
		gBoardId = DB_78X60_PCAC_REV2_ID;
#elif defined(DB_784MP_GP)
		gBoardId = DB_784MP_GP_ID;
#elif defined(RD_78460_CUSTOMER)
		gBoardId = RD_78460_CUSTOMER_ID;
#else
		mvOsPrintf("mvBoardIdSet: Board ID must be defined!\n");
		while (1) {
			continue;
		}
#endif
	}
}
 
MV_U32 mvBoardIdGet(MV_VOID)
{
	if (gBoardId == -1) {
		mvOsWarning();
		return INVALID_BAORD_ID;
	}

	return gBoardId;
}

MV_U8 mvBoardTwsiSatRGet(MV_U8 devNum, MV_U8 regNum)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	MV_U8 data;

	DB(mvOsPrintf("Board: Read S@R device read\n"));
	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(BOARD_DEV_TWSI_SATR, devNum);
	if (0xFF == twsiSlave.slaveAddr.address)
		return MV_ERROR;
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(BOARD_DEV_TWSI_SATR, devNum);

	twsiSlave.offset = regNum;
	twsiSlave.moreThen256 = MV_FALSE;
	twsiSlave.validOffset = MV_TRUE;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	if (MV_OK != mvTwsiRead(0, &twsiSlave, &data, 1)) {
		DB(mvOsPrintf("Board: Read S@R fail\n"));
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Read S@R succeded\n"));

	return data;
}

MV_STATUS mvBoardTwsiSatRSet(MV_U8 devNum, MV_U8 regNum, MV_U8 regVal)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;

	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(BOARD_DEV_TWSI_SATR, devNum);
	if (0xFF == twsiSlave.slaveAddr.address)
		return MV_ERROR;
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(BOARD_DEV_TWSI_SATR, devNum);
	twsiSlave.validOffset = MV_TRUE;
	DB(mvOsPrintf("Board: Write S@R device addr %x, type %x, data %x\n",
		      twsiSlave.slaveAddr.address, twsiSlave.slaveAddr.type, regVal));
	 
	twsiSlave.offset = regNum;
	twsiSlave.moreThen256 = MV_FALSE;
	 
	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	if (MV_OK != mvTwsiWrite(0, &twsiSlave, &regVal, 1)) {
		DB1(mvOsPrintf("Board: Write S@R fail\n"));
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Write S@R succeded\n"));

	return MV_OK;
}

MV_U8 mvBoardFabFreqGet(MV_VOID)
{
	MV_U8 sar0;
	MV_U8 sar1;
	MV_U32 boardId = mvBoardIdGet();

	sar0 = mvBoardTwsiSatRGet(2, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar0)
		return MV_ERROR;

	if (DB_784MP_GP_ID == boardId)
		return (sar0 & 0x1f);

	sar1 = mvBoardTwsiSatRGet(3, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar1)
		return MV_ERROR;

	return ( ((sar1 & 0x1) << 4) | ((sar0 & 0x1E) >> 1) );
}

MV_STATUS mvBoardFabFreqSet(MV_U8 freqVal)
{
	MV_U8 sar0;
	MV_U32 boardId = mvBoardIdGet();

	sar0 = mvBoardTwsiSatRGet(2, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar0)
		return MV_ERROR;
	if (DB_784MP_GP_ID == boardId) {
		sar0 &= ~(0x1F);
		sar0 |= (freqVal & 0x1F);
		if (MV_OK != mvBoardTwsiSatRSet(2, 0, sar0)) {
			DB1(mvOsPrintf("Board: Write FreqOpt S@R fail\n"));
			return MV_ERROR;
		}
		return MV_OK;
	}

	sar0 &= ~(0xF << 1);
	sar0 |= (freqVal & 0xF) << 1;
	if (MV_OK != mvBoardTwsiSatRSet(2, 0, sar0)) {
		DB1(mvOsPrintf("Board: Write FreqOpt S@R fail\n"));
		return MV_ERROR;
	}

	sar0 = mvBoardTwsiSatRGet(3, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar0)
		return MV_ERROR;

	sar0 &= ~(0x1);
	sar0 |= ( (freqVal >> 4) & 0x1);
	if (MV_OK != mvBoardTwsiSatRSet(3, 0, sar0)) {
		DB1(mvOsPrintf("Board: Write FreqOpt S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write FreqOpt S@R succeeded\n"));
	return MV_OK;
}
 
MV_U8 mvBoardCpuFreqGet(MV_VOID)
{
	MV_U8 sar;
	MV_U8 sarMsb;
	MV_U32 boardId = mvBoardIdGet();

	sar = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	if (DB_784MP_GP_ID == boardId) {
		return (sar & 0x0f);
	}

	sarMsb = mvBoardTwsiSatRGet(2, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	return (  ((sarMsb & 0x1) << 3) | ((sar & 0x1C) >> 2));
}

MV_STATUS mvBoardCpuFreqSet(MV_U8 freqVal)
{
	MV_U8 sar;
	MV_U32 boardId = mvBoardIdGet();

	sar = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	if (DB_784MP_GP_ID == boardId) {
		sar &= ~0x0f;
		sar |= (freqVal & 0x0f);
		if (MV_OK != mvBoardTwsiSatRSet(1, 0, sar)) {
			DB1(mvOsPrintf("Board: Write CpuFreq S@R fail\n"));
			return MV_ERROR;
		}
	}
	else{
		sar &= ~(0x7 << 2);
		sar |= (freqVal & 0x7) << 2;
		if (MV_OK != mvBoardTwsiSatRSet(1, 0, sar)) {
			DB1(mvOsPrintf("Board: Write CpuFreq S@R fail\n"));
			return MV_ERROR;
		}
		sar = mvBoardTwsiSatRGet(2, 0);
		if ((MV_8)MV_ERROR == (MV_8)sar)
			return MV_ERROR;
		sar &= ~(0x1);
		sar |= ( (freqVal >> 3) & 0x1);
		if (MV_OK != mvBoardTwsiSatRSet(2, 0, sar)) {
			DB1(mvOsPrintf("Board: Write CpuFreq S@R fail\n"));
			return MV_ERROR;
		}

		sar = mvBoardTwsiSatRGet(2, 0);
		if ((MV_8)MV_ERROR == (MV_8)sar)
			return MV_ERROR;

		sar &= ~(0x1);
		sar |= ( (freqVal >> 3) & 0x1);
		if (MV_OK != mvBoardTwsiSatRSet(2, 0, sar)) {
			DB1(mvOsPrintf("Board: Write CpuFreq S@R fail\n"));
			return MV_ERROR;
		}
	}

	DB(mvOsPrintf("Board: Write CpuFreq S@R succeeded\n"));
	return MV_OK;
}

MV_U8 mvBoardBootDevGet(MV_VOID)
{
	MV_U8 sar;

	sar = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	if (DB_784MP_GP_ID == mvBoardIdGet())
		sar = (sar >> 1);

	return (sar & 0x7);
}
 
MV_STATUS mvBoardBootDevSet(MV_U8 val)
{
	MV_U8 sar;
	MV_U32 boardId = mvBoardIdGet();

	sar = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	if (DB_784MP_GP_ID == boardId) {
		sar &= ~(0x7 << 1);
		sar |= ((val & 0x7) << 1);
	}
	else {
		sar &= ~(0x7);
		sar |= (val & 0x7);
	}
	if (MV_OK != mvBoardTwsiSatRSet(0, 0, sar)) {
		DB1(mvOsPrintf("Board: Write BootDev S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write BootDev S@R succeeded\n"));
	return MV_OK;
}
 
MV_U8 mvBoardBootDevWidthGet(MV_VOID)
{
	MV_U8 sar;
	MV_U32 boardId = mvBoardIdGet();

	sar = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	if (DB_784MP_GP_ID == boardId)
		return (sar & 1);

	return (sar & 0x18) >> 3;
}
 
MV_STATUS mvBoardBootDevWidthSet(MV_U8 val)
{
	MV_U8 sar;
	MV_U32 boardId = mvBoardIdGet();

	sar = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	if (DB_784MP_GP_ID == boardId) {
		sar &= ~(1);
		sar |= (val & 0x1);
	}
	else {
		sar &= ~(0x3 << 3);
		sar |= ((val & 0x3) << 3);
	}

	if (MV_OK != mvBoardTwsiSatRSet(0, 0, sar)) {
		DB1(mvOsPrintf("Board: Write BootDevWidth S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write BootDevWidth S@R succeeded\n"));
	return MV_OK;
}
 
MV_U8 mvBoardCpu0EndianessGet(MV_VOID)
{
	MV_U8 sar;
	if (DB_784MP_GP_ID == mvBoardIdGet())
		return 0;

	sar = mvBoardTwsiSatRGet(3, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	return (sar & 0x08) >> 3;
}
 
MV_STATUS mvBoardCpu0EndianessSet(MV_U8 val)
{
	MV_U8 sar;
	if (DB_784MP_GP_ID == mvBoardIdGet())
		return MV_OK;

	sar = mvBoardTwsiSatRGet(3, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	sar &= ~(0x1 << 3);
	sar |= ((val & 0x1) << 3);
	if (MV_OK != mvBoardTwsiSatRSet(3, 0, sar)) {
		DB1(mvOsPrintf("Board: Write Cpu0CoreMode S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write Cpu0CoreMode S@R succeeded\n"));
	return MV_OK;
}
 
MV_U8 mvBoardL2SizeGet(MV_VOID)
{
	MV_U8 sar;
	MV_U32 boardId = mvBoardIdGet();
	if (DB_784MP_GP_ID == boardId) {
		sar = mvBoardTwsiSatRGet(0, 0);
		if ((MV_8)MV_ERROR == (MV_8)sar)
			return MV_ERROR;
		return (((sar & 0x10)>>3)+ 1);
	}

	sar = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	return (sar & 0x3);
}
 
MV_STATUS mvBoardL2SizeSet(MV_U8 val)
{
	MV_U8 sar;
	if (DB_784MP_GP_ID == mvBoardIdGet()) {
		sar = mvBoardTwsiSatRGet(0, 0);
		if ((MV_8)MV_ERROR == (MV_8)sar)
			return MV_ERROR;
		sar &= ~(0x1 << 4);
		sar |= ((val & 0x2) << 3);
		if (MV_OK != mvBoardTwsiSatRSet(0, 0, sar)) {
			DB1(mvOsPrintf("Board: Write L2Size S@R fail\n"));
			return MV_ERROR;
		}
		return MV_OK;
	}

	sar = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	sar &= ~(0x3);
	sar |= (val & 0x3);
	if (MV_OK != mvBoardTwsiSatRSet(1, 0, sar)) {
		DB1(mvOsPrintf("Board: Write L2Size S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write L2Size S@R succeeded\n"));
	return MV_OK;
}
 
MV_U8 mvBoardCpuCoresNumGet(MV_VOID)
{
	MV_U8 sar;

	if (DB_784MP_GP_ID == mvBoardIdGet()) {
		sar = mvBoardTwsiSatRGet(1, 0);
		if ((MV_8)MV_ERROR == (MV_8)sar)
			return MV_ERROR;
		sar &=0x10;
		return (1+(sar >>3));
	}
	sar = mvBoardTwsiSatRGet(3, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	sar = (sar & 0x6) >> 1;
	if (sar == 1)
		sar = 2;
	else if (sar == 2)
		sar =1;
	return sar;
}
 
MV_STATUS mvBoardCpuCoresNumSet(MV_U8 val)
{
	MV_U8 sar;
	if (DB_784MP_GP_ID == mvBoardIdGet()) {
		sar = mvBoardTwsiSatRGet(1, 0);
		if ((MV_8)MV_ERROR == (MV_8)sar)
			return MV_ERROR;
		sar &=~0x10;
		val &= 2;
		sar |= (val<<3);
		if (MV_OK != mvBoardTwsiSatRSet(1, 0, sar)) {
			DB1(mvOsPrintf("Board: Write CpuCoreNum S@R fail\n"));
			return MV_ERROR;
		}
		DB(mvOsPrintf("Board: Write CpuCoreNum S@R succeeded\n"));
		return MV_OK;
	}
	sar = mvBoardTwsiSatRGet(3, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	 
	if (val == 1)
		val = 2;
	else if (val == 2)
		val =1;

	sar &= ~(0x3 << 1);
	sar |= ((val & 0x3) << 1);
	if (MV_OK != mvBoardTwsiSatRSet(3, 0, sar)) {
		DB1(mvOsPrintf("Board: Write CpuCoreNum S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write CpuCoreNum S@R succeeded\n"));
	return MV_OK;
}
 
MV_STATUS mvBoardConfIdSet(MV_U16 conf)
{
	if (MV_OK != mvBoardTwsiSatRSet(0, 1, conf)) {
		DB1(mvOsPrintf("Board: Write confID S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write confID S@R succeeded\n"));
	return MV_OK;
}

MV_U16 mvBoardConfIdGet(MV_VOID)
{
	MV_U8 sar;

	sar = mvBoardTwsiSatRGet(0, 1);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	return (sar & 0xFF);
}
 
MV_STATUS mvBoardPexCapabilitySet(MV_U16 conf)
{
	MV_U8 sar;
	sar = mvBoardTwsiSatRGet(1, 1);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	sar &= ~(0x1);
	sar |= (conf & 0x1);

	if (MV_OK != mvBoardTwsiSatRSet(1, 1, sar)) {
		DB(mvOsPrintf("Board: Write confID S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write confID S@R succeeded\n"));
	return MV_OK;
}
 
MV_U16 gPexCap = 0;
MV_U16 mvBoardPexCapabilityGet(MV_VOID)
{
	MV_U8 sar;
	MV_U32 boardId;

	if (gPexCap)
		return gPexCap;

	boardId = mvBoardIdGet();
	switch (boardId) {
	case DB_78X60_PCAC_ID:
	case RD_78460_NAS_ID:
	case RD_78460_CUSTOMER_ID:
	case DB_78X60_AMC_ID:
	case DB_78X60_PCAC_REV2_ID:
	case RD_78460_SERVER_ID:
	case RD_78460_SERVER_REV2_ID:
#ifdef MY_DEF_HERE
	case SYNO_AXP_4BAY_2BAY:
	case SYNO_AXP_2BAY:
	case SYNO_AXP_4BAY_RACK:
#endif
		sar = 0x1;  
		break;
	case DB_784MP_GP_ID:
	case DB_88F78XX0_BP_ID:
	case FPGA_88F78XX0_ID:
	case DB_88F78XX0_BP_REV2_ID:
	default:
		sar = mvBoardTwsiSatRGet(1, 1);
		break;
	}
	gPexCap = sar & 0x1;
	
	return (gPexCap);
}
 
MV_STATUS mvBoardPexModeSet(MV_U16 conf)
{
	MV_U8 sar;
	sar = mvBoardTwsiSatRGet(1, 1);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	sar &= ~(0x3 << 1);
	sar |= ((conf & 0x3) << 1);

	if (MV_OK != mvBoardTwsiSatRSet(1, 1, sar)) {
		DB(mvOsPrintf("Board: Write confID S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write confID S@R succeeded\n"));
	return MV_OK;
}
 
MV_U16 mvBoardPexModeGet(MV_VOID)
{
	MV_U8 sar;
#ifdef MY_DEF_HERE
	MV_U32 boardID = mvBoardIdGet();

	if (SYNO_AXP_4BAY_2BAY == boardID ||
	    SYNO_AXP_2BAY == boardID ||
	    SYNO_AXP_4BAY_RACK == boardID)
	{
		return 0x1;
	}
#endif

	sar = mvBoardTwsiSatRGet(1, 1);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	return (sar & 0x6) >> 1;

}
 
MV_STATUS mvBoardDramEccSet(MV_U16 ecc)
{
	MV_U8 sar;
	MV_U8 devNum;
	if (DB_784MP_GP_ID == mvBoardIdGet())
		devNum = 2;
	else
		devNum = 3;

	sar = mvBoardTwsiSatRGet(devNum, 1);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	sar &= ~(0x2);
	sar |= ((ecc & 0x1) << 1);

	if (MV_OK != mvBoardTwsiSatRSet(devNum, 1, sar)) {
		DB(mvOsPrintf("Board: Write eccID S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write eccID S@R succeeded\n"));
	return MV_OK;
}

MV_U16 mvBoardDramEccGet(MV_VOID)
{
	MV_U8 sar;
	MV_U8 devNum;
	if (DB_784MP_GP_ID == mvBoardIdGet())
		devNum = 2;
	else
		devNum = 3;

	sar = mvBoardTwsiSatRGet(devNum, 1);
	return ((sar & 0x2) >> 1);
}

MV_STATUS mvBoardDramBusWidthSet(MV_U16 dramBusWidth)
{
	MV_U8 sar;
	MV_U8 devNum;
	if (DB_784MP_GP_ID == mvBoardIdGet())
		devNum = 2;
	else
		devNum = 3;

	sar = mvBoardTwsiSatRGet(devNum, 1);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	sar &= ~(0x1);
	sar |= (dramBusWidth & 0x1);

	if (MV_OK != mvBoardTwsiSatRSet(devNum, 1, sar)) {
		DB(mvOsPrintf("Board: Write dramBusWidthID S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write dramBusWidthID S@R succeeded\n"));
	return MV_OK;
}

MV_U16 mvBoardDramBusWidthGet(MV_VOID)
{
	MV_U8 sar;

	MV_U8 devNum;
	if (DB_784MP_GP_ID == mvBoardIdGet())
		devNum = 2;
	else
		devNum = 3;

	sar = mvBoardTwsiSatRGet(devNum, 1);
	return (sar & 0x1);
}

MV_U8 mvBoardAltFabFreqGet(MV_VOID)
{
	MV_U8 sar0;
	if (DB_784MP_GP_ID == mvBoardIdGet())
		return 5;

	sar0 = mvBoardTwsiSatRGet(2, 1);
	if ((MV_8)MV_ERROR == (MV_8)sar0)
		return MV_ERROR;

	return (sar0 & 0x1F);
}
 
MV_STATUS mvBoardAltFabFreqSet(MV_U8 freqVal)
{
	if (DB_784MP_GP_ID == mvBoardIdGet())
		return MV_OK;

	if (MV_OK != mvBoardTwsiSatRSet(2, 1, freqVal)) {
		DB1(mvOsPrintf("Board: Write Alt FreqOpt S@R fail\n"));
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Write Alt FreqOpt S@R succeeded\n"));
	return MV_OK;
}
 
MV_STATUS mvBoardMppModulesScan(void)
{
	MV_U8 regVal;
	MV_TWSI_SLAVE twsiSlave;
	MV_U32 boardId = mvBoardIdGet();

	if ( (boardId == DB_88F78XX0_BP_ID) || (boardId == DB_88F78XX0_BP_REV2_ID) ) {
		twsiSlave.slaveAddr.address = MV_BOARD_MPP_MODULE_ADDR;
		twsiSlave.slaveAddr.type = MV_BOARD_MPP_MODULE_ADDR_TYPE;
		twsiSlave.validOffset = MV_TRUE;
		twsiSlave.offset = 0;
		twsiSlave.moreThen256 = MV_FALSE;
		if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
			switch (regVal) {
			case MV_BOARD_LCD_DVI_MODULE_ID:
				BOARD_INFO(boardId)->pBoardModTypeValue->boardMppMod = MV_BOARD_LCD_DVI;
				return MV_OK;

			case MV_BOARD_MII_GMII_MODULE_ID:
				BOARD_INFO(boardId)->pBoardModTypeValue->boardMppMod = MV_BOARD_MII_GMII;
			    BOARD_INFO(boardId)->pBoardMacInfo[0].boardEthSmiAddr = 0x8;

				return MV_OK;

			case MV_BOARD_TDM_MODULE_ID:
				 
				BOARD_INFO(boardId)->boardTdmInfoIndex = BOARD_TDM_SLIC_OTHER;
				BOARD_INFO(boardId)->pBoardModTypeValue->boardMppMod = MV_BOARD_TDM_32CH;
				return MV_OK;

			default:
				BOARD_INFO(boardId)->pBoardModTypeValue->boardMppMod = MV_BOARD_OTHER;
				DB1(mvOsPrintf("mvBoardMppModulesScan: Unsupported module!\n"));
				break;
			}
		}
	}

	return MV_OK;
}

MV_STATUS mvBoardOtherModulesScan(void)
{
	MV_U8 regVal;
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	MV_U32 boardId = mvBoardIdGet();

	if ( (boardId == DB_88F78XX0_BP_ID) || (boardId == DB_88F78XX0_BP_REV2_ID) ) {
		 
		BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod |= MV_BOARD_NONE;

        slave.type = ADDR7_BIT;
        slave.address = 0;
        mvTwsiInit(0, TWSI_SPEED , mvBoardTclkGet(), &slave, 0);

		twsiSlave.slaveAddr.address = MV_BOARD_PEX_MODULE_ADDR;
		twsiSlave.slaveAddr.type = MV_BOARD_PEX_MODULE_ADDR_TYPE;
		twsiSlave.validOffset = MV_TRUE;
		twsiSlave.offset = 0;
		twsiSlave.moreThen256 = MV_FALSE;
		if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
			if (regVal == MV_BOARD_PEX_MODULE_ID) {
				DB(mvOsPrintf("mvBoardOtherModulesScan: " "PEX module DETECTED!\n"));
				BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod |= MV_BOARD_PEX;
			} else {
				DB(mvOsPrintf("mvBoardOtherModulesScan: " "Unknown ID @ PEX module address!\n"));
				BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod |= MV_BOARD_UNKNOWN;
			}
		}

		twsiSlave.slaveAddr.address = MV_BOARD_SETM_MODULE_ADDR;
		twsiSlave.slaveAddr.type = MV_BOARD_SETM_MODULE_ADDR_TYPE;
		twsiSlave.validOffset = MV_TRUE;
		twsiSlave.offset = 0;
		twsiSlave.moreThen256 = MV_FALSE;
		if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
			if (regVal == MV_BOARD_SETM_MODULE_ID) {
				DB(mvOsPrintf("mvBoardOtherModulesScan: " "SETM module DETECTED!\n"));
				BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod |= MV_BOARD_SETM;
			} else {
				DB(mvOsPrintf("mvBoardOtherModulesScan: " "Unknown ID @ PEX module address!\n"));
				BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod |= MV_BOARD_UNKNOWN;
			}
		}

		twsiSlave.slaveAddr.address = MV_BOARD_LVDS_MODULE_ADDR;
		twsiSlave.slaveAddr.type = MV_BOARD_LVDS_MODULE_ADDR_TYPE;
		if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
			if (regVal == MV_BOARD_LVDS_MODULE_ID) {
				BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod |= MV_BOARD_LVDS;
				mvCpuIfLvdsPadsEnable(MV_TRUE);
			} else {
				DB(mvOsPrintf("mvBoardOtherModulesScan: " "Unknown ID @ LVDS module address!\n"));
				BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod |= MV_BOARD_UNKNOWN;
			}
		}
	} else if (boardId == RD_78460_NAS_ID) {
		if ((MV_REG_READ(GPP_DATA_IN_REG(2)) & MV_GPP66) == 0x0) {
			DB(mvOsPrintf("mvBoardOtherModulesScan: SWITCH module DETECTED!\n"));
			BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod |= MV_BOARD_SWITCH;
		}
	}

	return MV_OK;
}

MV_BOOL mvBoardIsPexModuleConnected(void)
{
	MV_U32 boardId = mvBoardIdGet();

	if ( (boardId != DB_88F78XX0_BP_ID) && (boardId != DB_88F78XX0_BP_REV2_ID) )
		DB(mvOsPrintf("mvBoardIsPexModuleConnected: Unsupported board!\n"));
	else if (BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod & MV_BOARD_PEX)
		return MV_TRUE;

	return MV_FALSE;
}

MV_BOOL mvBoardIsSetmModuleConnected(void)
{
	MV_U32 boardId = mvBoardIdGet();

	if ( (boardId != DB_88F78XX0_BP_ID) && (boardId != DB_88F78XX0_BP_REV2_ID) )
		DB(mvOsPrintf("mvBoardIsSetmModuleConnected: Unsupported board!\n"));
	else if (BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod & MV_BOARD_SETM)
		return MV_TRUE;
	return MV_FALSE;
}
 
MV_BOOL mvBoardIsSwitchModuleConnected(void)
{
	MV_U32 boardId = mvBoardIdGet();

	if (boardId != RD_78460_NAS_ID)
		DB(mvOsPrintf("mvBoardIsSwitchModuleConnected: Unsupported board!\n"));
	else if (BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod & MV_BOARD_SWITCH)
		return MV_TRUE;

	return MV_FALSE;
}

MV_BOOL mvBoardIsLvdsModuleConnected(void)
{
	MV_U32 boardId = mvBoardIdGet();

	if ( (boardId != DB_88F78XX0_BP_ID) && (boardId != DB_88F78XX0_BP_REV2_ID) )
		DB(mvOsPrintf("mvBoardIsLvdsModuleConnected: Unsupported board!\n"));
	else if (BOARD_INFO(boardId)->pBoardModTypeValue->boardOtherMod & MV_BOARD_LVDS)
		return MV_TRUE;

	return MV_FALSE;
}

MV_BOOL mvBoardIsLcdDviModuleConnected(void)
{
	MV_U32 boardId = mvBoardIdGet();

	if ( (boardId != DB_88F78XX0_BP_ID) && (boardId != DB_88F78XX0_BP_REV2_ID) )
		DB(mvOsPrintf("mvBoardIsLcdDviModuleConnected: Unsupported board!\n"));
	else if (BOARD_INFO(boardId)->pBoardModTypeValue->boardMppMod == MV_BOARD_LCD_DVI)
		return MV_TRUE;

	return MV_FALSE;
}

MV_BOOL mvBoardIsGMIIModuleConnected(void)
{
	MV_U32 boardId = mvBoardIdGet();

	if ( (boardId != DB_88F78XX0_BP_ID) && (boardId != DB_88F78XX0_BP_REV2_ID) )
		DB(mvOsPrintf("mvBoardIsGMIIModuleConnected: Unsupported board!\n"));
	else if (BOARD_INFO(boardId)->pBoardModTypeValue->boardMppMod == MV_BOARD_MII_GMII)
		return MV_TRUE;

	return MV_FALSE;
}

MV_STATUS mvBoardTwsiMuxChannelSet(MV_U8 muxChNum)
{
	static MV_U8 currChNum = 0xFF;
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;

	if (currChNum == muxChNum)
		return MV_OK;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(BOARD_TWSI_MUX, 0);
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(BOARD_TWSI_MUX, 0);
	twsiSlave.validOffset = 0;
	twsiSlave.offset = 0;
	twsiSlave.moreThen256 = MV_FALSE;
	muxChNum += 4;
	return mvTwsiWrite(0, &twsiSlave, &muxChNum, 1);
}

MV_STATUS mvBoardTwsiReadByteThruMux(MV_U8 muxChNum, MV_U8 chNum, MV_TWSI_SLAVE *pTwsiSlave, MV_U8 *data)
{
	MV_STATUS res;

	res = mvBoardTwsiMuxChannelSet(muxChNum);
	if (res == MV_OK)
		res = mvTwsiRead(chNum, pTwsiSlave, data, 1);

	return res;
}

MV_VOID mvBoardSerdesZ1ASupport(void)
{
	gSerdesZ1AMode = 1;
}

MV_32 mvBoardSmiScanModeGet(MV_U32 switchIdx)
{
	 MV_U32 boardId = mvBoardIdGet();

	 if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSmiScanModeGet: Board unknown.\n");
		return -1;
        }

	return BOARD_INFO(boardId)->pSwitchInfo[switchIdx].smiScanMode;
}
 
MV_U32 mvBoardSledCpuNumGet(MV_VOID)
{
	MV_U32 reg;

	reg = MV_REG_READ(GPP_DATA_IN_REG(0));

	return ((reg & 0xF0000) >> 16);
}

MV_BOARD_PEX_INFO *mvBoardPexInfoGet(void)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	switch (boardId) {
	case DB_88F78XX0_BP_ID:
	case RD_78460_SERVER_ID:
	case RD_78460_SERVER_REV2_ID:
	case DB_78X60_PCAC_ID:
	case FPGA_88F78XX0_ID:
	case DB_88F78XX0_BP_REV2_ID:
	case RD_78460_NAS_ID:
	case DB_784MP_GP_ID:
	case RD_78460_CUSTOMER_ID:
	case DB_78X60_AMC_ID:
	case DB_78X60_PCAC_REV2_ID:
#ifdef MY_DEF_HERE
	case SYNO_AXP_4BAY_2BAY:
	case SYNO_AXP_2BAY:
	case SYNO_AXP_4BAY_RACK:
#endif
		return &BOARD_INFO(boardId)->boardPexInfo;
		break;
	default:
		DB(mvOsPrintf("mvBoardSerdesCfgGet: Unsupported board!\n"));
		return NULL;
	}
}
