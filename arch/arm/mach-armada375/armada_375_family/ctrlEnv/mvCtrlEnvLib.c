#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"
#include "mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/mvCtrlEthCompLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"
#include "mvSysEthConfig.h"

#include "pex/mvPex.h"
#include "pex/mvPexRegs.h"

#if defined(MV_INCLUDE_XOR)
#include "xor/mvXor.h"
#endif

#if defined(MV_INCLUDE_SATA)
#include "sata/CoreDriver/mvSata.h"
#endif
#if defined(MV_INCLUDE_USB)
#include "usb/mvUsb.h"
#endif

#if defined(MV_INCLUDE_TDM)
#include "mvSysTdmConfig.h"
#endif

#include "ddr2_3/mvDramIfRegs.h"

#undef MV_DEBUG
#ifdef MV_DEBUG
#define DB(x)   x
#else
#define DB(x)
#endif

#define MV_INVALID_CTRL_REV     0xff

typedef struct _ctrlEnvInfo {
	MV_U16 ctrlModel;
	MV_U8 ctrlRev;
} CTRL_ENV_INFO;

CTRL_ENV_INFO ctrlEnvInfo = {};

MV_U32 satrOptionsConfig[MV_SATR_READ_MAX_OPTION];
MV_U32 boardOptionsConfig[MV_CONFIG_TYPE_MAX_OPTION];
MV_32 satrOptionsInitialized = -1;  

MV_BOARD_SATR_INFO boardSatrInfo[] = MV_SAR_INFO;

MV_U32 mvCtrlGetCpuNum(MV_VOID)
{
	MV_U32 cpu1Enabled;

	cpu1Enabled = mvCtrlSatRRead(MV_SATR_CPU1_ENABLE);
	if (cpu1Enabled == MV_ERROR) {
		DB(mvOsPrintf("%s: Error: MV_SATR_CPU1_ENABLE is not active for board (using default)\n", __func__));
		return 0;
	} else
		return cpu1Enabled;
}

MV_BOOL mvCtrlIsValidSatR(MV_VOID)
{
	MV_FREQ_MODE cpuFreqMode;
	MV_U32 cpuFreqSatRMode =  mvCtrlSatRRead(MV_SATR_CPU_DDR_L2_FREQ);

	if (mvCtrlFreqModeGet(cpuFreqSatRMode, &cpuFreqMode) == MV_OK)
		return MV_TRUE;
	else
		return MV_FALSE;
}
 
MV_STATUS mvCtrlFreqModeGet(MV_U32 freqModeSatRValue, MV_FREQ_MODE *freqMode)
{
	MV_FREQ_MODE freqTable[] = MV_USER_SAR_FREQ_MODES;
	MV_U32 i, maxFreqModes = mvBoardFreqModesNumGet();

	for (i = 0; i < maxFreqModes; i++) {
		if (freqModeSatRValue == freqTable[i].id) {
			*freqMode = freqTable[i];
			return MV_OK;
		}
	}

	return MV_ERROR;
}

#ifdef MV_INCLUDE_PEX
MV_STATUS mvCtrlUpdatePexId(MV_VOID)
{
	return MV_ERROR;
}

#endif

#define MV_6720_INDEX		0
#define MV_67xx_INDEX_MAX	1

static MV_U32 mvCtrlDevIdIndexGet(MV_U32 devId)
{
	return MV_6720_INDEX;
}

static MV_VOID mvCtrlPexConfig(MV_VOID)
{
	MV_U8 pexUnit;
	MV_U32 pexIfNum = mvCtrlSocUnitInfoNumGet(PEX_UNIT_ID);

	MV_BOARD_PEX_INFO *boardPexInfo = mvBoardPexInfoGet();

	memset(boardPexInfo, 0, sizeof(MV_BOARD_PEX_INFO));

	for (pexUnit = 0; pexUnit < pexIfNum; pexUnit++) {
		boardPexInfo->pexUnitCfg[pexUnit] = PEX_BUS_MODE_X1;
		boardPexInfo->pexMapping[pexUnit] = pexUnit;
	}

	boardPexInfo->boardPexIfNum = pexIfNum;
}

MV_UNIT_ID mvCtrlSocUnitNums[MAX_UNITS_ID][MV_67xx_INDEX_MAX] = {
 
  { 1, },
  { 2, },
  { 2, },
  { 2, },
  { 1, },
  { 0, },
  { 2, },
  { 2, },
  { 1, },
  { 2, },
  { 2, },
  { 2, },
  { 1, },
  { 1, },
  { 0, },
  { 1, },
  { 1, },
  { 1, },
  { 2, },
  { 1, },
};

MV_U32 mvCtrlSocUnitInfoNumGet(MV_UNIT_ID unit)
{
	MV_U32 devId, devIdIndex;

	if (unit >= MAX_UNITS_ID) {
		mvOsPrintf("%s: Error: Wrong unit type (%u)\n", __func__, unit);
		return 0;
	}

	devId = mvCtrlModelGet();
	devIdIndex = mvCtrlDevIdIndexGet(devId);
	return mvCtrlSocUnitNums[unit][devIdIndex];
}

MV_STATUS mvCtrlEnvInit(MV_VOID)
{
	MV_U32 i, gppMask;

	MV_REG_WRITE(mvCtrlMppRegGet(1), GROUP1_DEFAULT_MPP8_15_I2C);
	MV_REG_WRITE(mvCtrlMppRegGet(7), GROUP1_DEFAULT_MPP56_63_I2C);

	mvCtrlSatrInit(0);

	if (mvBoardConfigAutoDetectEnabled()) {
		mvBoardInfoUpdate();
	}

	mvCtrlPexConfig();

	mvBoardConfigWrite();

	if (mvBoardEthComplexConfigGet() & MV_ETHCOMP_GE_MAC1_2_PON_ETH_SERDES_SFP)
		mvBoardSgmiiSfp1TxSet(MV_TRUE);

	for (i = 0; i < MV_GPP_MAX_GROUP; i++) {
		MV_REG_WRITE(GPP_INT_MASK_REG(i), 0x0);
		MV_REG_WRITE(GPP_INT_LVL_REG(i), 0x0);
	}

	for (i = 0; i < MV_GPP_MAX_GROUP; i++)
		MV_REG_WRITE(GPP_INT_CAUSE_REG(i), 0x0);

	for (i = 0; i < MV_GPP_MAX_GROUP; i++) {
		gppMask = mvBoardGpioIntMaskGet(i);
		mvGppTypeSet(i, gppMask, (MV_GPP_IN & gppMask));
		mvGppPolaritySet(i, gppMask, (MV_GPP_IN_INVERT & gppMask));
	}

	MV_REG_BIT_SET(PUP_EN_REG, BIT4);

#ifdef MV_NOR_BOOT
	 
	MV_REG_BIT_SET(PUP_EN_REG, BIT6);
#endif
	 
	MV_REG_BIT_SET(SOC_DEV_MUX_REG, BIT0);  

	MV_REG_BIT_RESET(SOC_DEV_MUX_REG, BIT27);

	MV_REG_BIT_RESET(SOC_COHERENCY_FABRIC_CTRL_REG, BIT8);

	MV_REG_BIT_SET(SATAHC_LED_CONFIGURATION_REG, BIT3);

	return MV_OK;
}

MV_STATUS mvCtrlSatRWrite(MV_SATR_TYPE_ID satrWriteField, MV_SATR_TYPE_ID satrReadField, MV_U8 val)
{
	MV_BOARD_SATR_INFO satrInfo;
	MV_U8 readValue, verifyValue, i2cRegNum = 0;

	if (satrOptionsInitialized < 2)
		return MV_ERROR;

	if (satrReadField >= MV_SATR_READ_MAX_OPTION ||
		satrWriteField >= MV_SATR_WRITE_MAX_OPTION) {
		mvOsPrintf("%s: Error: wrong MV_SATR_TYPE_ID field value (%d).\n", __func__ ,satrWriteField);
		return MV_ERROR;
	}

	if (mvBoardSatrInfoConfig(satrWriteField, &satrInfo, MV_FALSE) != MV_OK) {
		mvOsPrintf("%s: Error: Requested S@R field is not relevant for this board\n", __func__);
		return MV_ERROR;
	}

	if (satrWriteField == MV_SATR_WRITE_DDR_BUS_WIDTH)
		i2cRegNum = 1;

	if (mvBoardTwsiGet(BOARD_DEV_TWSI_SATR, satrInfo.regNum, i2cRegNum, &readValue) != MV_OK) {
		mvOsPrintf("%s: Error: Read from S@R failed\n", __func__);
		return MV_ERROR;
	}

	if (satrWriteField == MV_SATR_WRITE_CPU_FREQ)
		val = mvReverseBits(val) >> 3 ;

	readValue &= ~(satrInfo.mask);              
	readValue |= (val <<  satrInfo.offset);     

	if (mvBoardTwsiSet(BOARD_DEV_TWSI_SATR, satrInfo.regNum, i2cRegNum, readValue) != MV_OK) {
		mvOsPrintf("%s: Error: Write to S@R failed\n", __func__);
		return MV_ERROR;
	}

	if (mvBoardTwsiGet(BOARD_DEV_TWSI_SATR, satrInfo.regNum, i2cRegNum, &verifyValue) != MV_OK) {
		mvOsPrintf("%s: Error: 2nd Read from S@R failed\n", __func__);
		return MV_ERROR;
	}

	if (readValue != verifyValue) {
		mvOsPrintf("%s: Error: Write to S@R failed : written value doesn't match\n", __func__);
		return MV_ERROR;
	}

	if (satrWriteField == MV_SATR_WRITE_CPU_FREQ)
		val = mvReverseBits(val) >> 3 ;

	satrOptionsConfig[satrReadField] = val;
	return MV_OK;
}

MV_U32 mvCtrlSatRRead(MV_SATR_TYPE_ID satrField)
{
	MV_BOARD_SATR_INFO satrInfo;

	if (satrOptionsInitialized < 1)
		mvCtrlSatrInit(1);
	if ((satrField == MV_SATR_DDR_BUS_WIDTH) && (satrOptionsInitialized < 2))
		return MV_ERROR;

	if (satrField < MV_SATR_READ_MAX_OPTION &&
			mvBoardSatrInfoConfig(satrField, &satrInfo, MV_TRUE) == MV_OK)
		return satrOptionsConfig[satrField];
	else
		return MV_ERROR;
}

MV_VOID mvCtrlSmiMasterSet(MV_SMI_CTRL smiCtrl)
{
	MV_U32 smiCtrlValue, mppValue = MV_REG_READ(mvCtrlMppRegGet(4));

	switch (smiCtrl) {
	case SWITCH_SMI_CTRL:
		smiCtrlValue = A375_MPP32_39_SWITCH_SMI_CTRL_VAL;
		break;
	case NO_SMI_CTRL:
		smiCtrlValue = A375_MPP32_39_NO_SMI_CTRL_VAL;
		break;
	case CPU_SMI_CTRL:
	default:
		smiCtrlValue = A375_MPP32_39_CPU_SMI_CTRL_VAL;
		break;
	}

	mppValue &= ~A375_MPP32_39_EXT_SMI_MASK;
	mppValue |= smiCtrlValue;

	MV_REG_WRITE(mvCtrlMppRegGet(4), mppValue);
}

MV_STATUS mvCtrlCpuDdrL2FreqGet(MV_FREQ_MODE *freqMode)
{
	MV_U32 freqModeSatRValue = mvCtrlSatRRead(MV_SATR_CPU_DDR_L2_FREQ);

	if (freqMode == NULL) {
		mvOsPrintf("%s: Error: NULL pointer parameter\n", __func__);
		return MV_ERROR;
	}

	if (MV_ERROR != freqModeSatRValue)
		return mvCtrlFreqModeGet(freqModeSatRValue, freqMode);

	DB(mvOsPrintf("%s: Error Read from S@R fail\n", __func__));
	return MV_ERROR;

}

MV_U32 mvCtrlSysConfigGet(MV_CONFIG_TYPE_ID configField)
{
	MV_BOARD_CONFIG_TYPE_INFO configInfo;

	if (!mvBoardConfigAutoDetectEnabled()) {
		mvOsPrintf("%s: Error reading board configuration", __func__);
		mvOsPrintf("- Auto detection is disabled\n");
		return MV_ERROR;
	}

	if (configField < MV_CONFIG_TYPE_MAX_OPTION &&
		mvBoardConfigTypeGet(configField, &configInfo) != MV_TRUE) {
		mvOsPrintf("%s: Error: Requested board config", __func__);
		mvOsPrintf("is not valid for this board(%d)\n", configField);
		return -1;
	}

	return boardOptionsConfig[configField];

}

MV_VOID mvCtrlSatrInit(MV_U32 early)
{
	MV_U32 satrVal[2];
	MV_BOARD_SATR_INFO satrInfo;
	MV_U32 i;
	MV_U8 readValue;

	if (satrOptionsInitialized < 1) {
		 
		memset(&satrOptionsConfig, 0x0, sizeof(MV_U32) * MV_SATR_READ_MAX_OPTION);

		satrVal[0] = MV_REG_READ(MPP_SAMPLE_AT_RESET(0));
		satrVal[1] = MV_REG_READ(MPP_SAMPLE_AT_RESET(1));

		for (i = 0; i < MV_SATR_READ_MAX_OPTION; i++)
			if (mvBoardSatrInfoConfig(i, &satrInfo, MV_TRUE) == MV_OK)
				satrOptionsConfig[satrInfo.satrId] =
					((satrVal[satrInfo.regNum]  & (satrInfo.mask)) >> (satrInfo.offset));

		satrOptionsInitialized = 1;
	}

	if (early)
		return;

	if (mvBoardIdGet() == DB_6720_ID) {
		 
		if (mvBoardSatrInfoConfig(MV_SATR_WRITE_DDR_BUS_WIDTH, &satrInfo, MV_FALSE) != MV_OK)
			mvOsPrintf("%s: Error: DDR_BUS_WIDTH field is not relevant for this board\n", __func__);

		if (mvBoardTwsiGet(BOARD_DEV_TWSI_SATR, satrInfo.regNum, 1, &readValue) != MV_OK)
			mvOsPrintf("%s: Error: Read DDR_BUS_WIDTH from S@R failed\n", __func__);

		satrOptionsConfig[MV_SATR_DDR_BUS_WIDTH] = ((readValue  & (satrInfo.mask)) >> (satrInfo.offset));

		satrOptionsInitialized = 2;
	}
}

MV_U32 mvCtrlDevFamilyIdGet(MV_U16 ctrlModel)
{
	return MV_88F67X0;
}

MV_U32 mvCtrlMppRegGet(MV_U32 mppGroup)
{
	MV_U32 ret;

	if (mppGroup >= MV_MPP_MAX_GROUP)
		mppGroup = 0;

	ret = MPP_CONTROL_REG(mppGroup);

	return ret;
}

MV_U32 mvCtrlLaneSelectorGet(MV_U32 laneNum)
{
	MV_U32 laneUnits[4][4] = {{ PEX_UNIT_ID },
				  { PEX_UNIT_ID, SGMII_UNIT_ID, SATA_UNIT_ID },
				  { SGMII_UNIT_ID, SATA_UNIT_ID },
				  { USB3_UNIT_ID, SGMII_UNIT_ID } };

	MV_U32  selector = (laneNum == 0 ? 0 : MV_REG_READ(MV_COMMON_PHY_REGS_OFFSET));

	if (laneNum >= 4)
		return MV_ERROR;

	selector = (selector & SERDES_LANE_MASK(laneNum)) >> SERDES_LANE_OFFS(laneNum);
	return laneUnits[laneNum][selector];
}

#if defined(MV_INCLUDE_PEX)
 
MV_U32 mvCtrlPexMaxIfGet(MV_VOID)
{
	MV_U32 pexMaxIfNum = mvCtrlSocUnitInfoNumGet(PEX_UNIT_ID);

	if (mvCtrlRevGet() >= MV_88F672X_A0_ID && mvCtrlLaneSelectorGet(1) != PEX_UNIT_ID)
		pexMaxIfNum--;

	return pexMaxIfNum;
}

#endif

MV_U32 mvCtrlPexMaxUnitGet(MV_VOID)
{
	return mvCtrlSocUnitInfoNumGet(PEX_UNIT_ID);
}

MV_U32 mvCtrlPexActiveUnitNumGet(MV_VOID)
{
	return mvCtrlSocUnitInfoNumGet(PEX_UNIT_ID);
}

#if defined(MV_INCLUDE_PCI)
 
#ifndef mvCtrlPciMaxIfGet
MV_U32 mvCtrlPciMaxIfGet(MV_VOID)
{
	return 1;
}

#endif
#endif

MV_U32 mvCtrlEthMaxPortGet(MV_VOID)
{
	return MV_ETH_MAX_PORTS;
}

#if defined(MV_INCLUDE_SATA)
 
MV_U32 mvCtrlSataMaxPortGet(MV_VOID)
{
	MV_U32 sataMaxNum = mvCtrlSocUnitInfoNumGet(SATA_UNIT_ID);

	if (mvCtrlRevGet() >= MV_88F672X_A0_ID && mvCtrlLaneSelectorGet(2) != SATA_UNIT_ID)
		sataMaxNum--;

	if (mvCtrlRevGet() >= MV_88F672X_A0_ID && mvCtrlLaneSelectorGet(1) != SATA_UNIT_ID)
		sataMaxNum--;

	return sataMaxNum;
}

#endif

#if defined(MV_INCLUDE_XOR)
 
MV_U32 mvCtrlXorMaxChanGet(MV_VOID)
{
	return mvCtrlSocUnitInfoNumGet(XOR_UNIT_ID);
}

MV_U32 mvCtrlXorMaxUnitGet(MV_VOID)
{
	return mvCtrlSocUnitInfoNumGet(XOR_UNIT_ID);
}

#endif

#if defined(MV_INCLUDE_USB)
 
MV_U32 mvCtrlUsbMaxGet(void)
{
	return mvCtrlSocUnitInfoNumGet(USB_UNIT_ID);
}

MV_U32 mvCtrlUsb3MaxGet(void)
{
	MV_U32 usb3MaxNum = mvCtrlSocUnitInfoNumGet(USB3_UNIT_ID);

	if (mvCtrlRevGet() >= MV_88F672X_A0_ID && mvCtrlLaneSelectorGet(3) != USB3_UNIT_ID)
		usb3MaxNum--;

	return usb3MaxNum;
}
#endif

#if defined(MV_INCLUDE_SDIO)
 
MV_U32 mvCtrlSdioSupport(MV_VOID)
{
	return mvCtrlSocUnitInfoNumGet(SDIO_UNIT_ID) ? MV_TRUE : MV_FALSE;
}

#endif

MV_U32 mvCtrlTdmSupport(MV_VOID)
{
	return mvCtrlSocUnitInfoNumGet(TDM_UNIT_ID) ? MV_TRUE : MV_FALSE;
}

MV_U32 mvCtrlTdmMaxGet(MV_VOID)
{
	return mvCtrlSocUnitInfoNumGet(TDM_UNIT_ID);
}

MV_TDM_UNIT_TYPE mvCtrlTdmUnitTypeGet(MV_VOID)
{
	return TDM_UNIT_2CH;
}

MV_U32 mvCtrlTdmUnitIrqGet(MV_VOID)
{
	return MV_TDM_IRQ_NUM;
}

MV_U16 mvCtrlModelGet(MV_VOID)
{
	return MV_6720_DEV_ID;
}

MV_U8 mvCtrlRevGet(MV_VOID)
{
	MV_U32 value;

	value = MV_REG_READ(DEV_VERSION_ID_REG);
	return  ((value & (REVISON_ID_MASK) ) >> REVISON_ID_OFFS);
}

MV_STATUS mvCtrlNameGet(char *pNameBuff)
{
	mvOsSPrintf(pNameBuff, "%s%x", SOC_NAME_PREFIX, mvCtrlModelGet());
	return MV_OK;
}

MV_U32 mvCtrlModelRevGet(MV_VOID)
{
	return (mvCtrlModelGet() << 16) | mvCtrlRevGet();
}

MV_VOID mvCtrlRevNameGet(char *pNameBuff)
{
	MV_U32 revId;
	char *revArray[] = MV_88F672X_ID_ARRAY;

	revId = mvCtrlRevGet();

	switch (revId) {
	case MV_88F6720_Z1_ID:
	case MV_88F6720_Z2_ID:
	case MV_88F6720_Z3_ID:
	case MV_88F672X_A0_ID:
			mvOsSPrintf(pNameBuff, " Rev %s", revArray[revId]);
			return;
	default:
		mvOsPrintf("%s: Error: Failed to read Revision ID\n", __func__);
	}
}

MV_VOID mvCtrlModelRevNameGet(char *pNameBuff)
{
	mvCtrlNameGet(pNameBuff);
	mvCtrlRevNameGet(pNameBuff + strlen(pNameBuff));
}

static const char *cntrlName[] = TARGETS_NAME_ARRAY;

const MV_8 *mvCtrlTargetNameGet(MV_TARGET target)
{
	if (target >= MAX_TARGETS)
		return "target unknown";

	return cntrlName[target];
}

#if defined(MV_INCLUDE_PEX)
static MV_VOID mvCtrlPexAddrDecShow(MV_VOID)
{
	MV_PEX_BAR pexBar;
	MV_PEX_DEC_WIN win;
	MV_U32 pexIf;
	MV_U32 bar, winNum;
	MV_BOARD_PEX_INFO *boardPexInfo = mvBoardPexInfoGet();
	MV_U32 pexHWInf = 0;

	for (pexIf = 0; pexIf < boardPexInfo->boardPexIfNum; pexIf++) {
		pexHWInf = pexIf;

		if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexHWInf))
			continue;
		mvOsOutput("\n");
		mvOsOutput("PEX%d:\n", pexHWInf);
		mvOsOutput("-----\n");

		mvOsOutput("\nPex Bars\n\n");

		for (bar = 0; bar < PEX_MAX_BARS; bar++) {
			memset(&pexBar, 0, sizeof(MV_PEX_BAR));

			mvOsOutput("%s ", pexBarNameGet(bar));

			if (mvPexBarGet(pexHWInf, bar, &pexBar) == MV_OK) {
				if (pexBar.enable) {
					mvOsOutput("base %08x, ", pexBar.addrWin.baseLow);
					if (pexBar.addrWin.size == 0)
						mvOsOutput("size %3dGB ", 4);
					else
						mvSizePrint(pexBar.addrWin.size);
					mvOsOutput("\n");
				} else
					mvOsOutput("disable\n");
			}
		}
		mvOsOutput("\nPex Decode Windows\n\n");

		for (winNum = 0; winNum < PEX_MAX_TARGET_WIN - 2; winNum++) {
			memset(&win, 0, sizeof(MV_PEX_DEC_WIN));

			mvOsOutput("win%d - ", winNum);

			if (mvPexTargetWinRead(pexHWInf, winNum, &win) == MV_OK) {
				if (win.winInfo.enable) {
					mvOsOutput("%s base %08x, ",
						   mvCtrlTargetNameGet(mvCtrlTargetByWinInfoGet(&win.winInfo)),
						   win.winInfo.addrWin.baseLow);
					mvOsOutput("....");
					mvSizePrint(win.winInfo.addrWin.size);

					mvOsOutput("\n");
				} else
					mvOsOutput("disable\n");
			}
		}

		memset(&win, 0, sizeof(MV_PEX_DEC_WIN));

		mvOsOutput("default win - ");

		if (mvPexTargetWinRead(pexHWInf, MV_PEX_WIN_DEFAULT, &win) == MV_OK) {
			mvOsOutput("%s ", mvCtrlTargetNameGet(win.target));
			mvOsOutput("\n");
		}
		memset(&win, 0, sizeof(MV_PEX_DEC_WIN));

		mvOsOutput("Expansion ROM - ");

		if (mvPexTargetWinRead(pexHWInf, MV_PEX_WIN_EXP_ROM, &win) == MV_OK) {
			mvOsOutput("%s ", mvCtrlTargetNameGet(win.target));
			mvOsOutput("\n");
		}
	}
}

#endif

static void mvUnitAddrDecShow(MV_U8 numUnits, MV_UNIT_ID unitId,
			      const char *name, MV_WIN_GET_FUNC_PTR winGetFuncPtr)
{
	MV_UNIT_WIN_INFO win;
	MV_U32 unit, i;

	for (unit = 0; unit < numUnits; unit++) {
		if (MV_FALSE == mvCtrlPwrClckGet(unitId, unit))
			continue;
		mvOsOutput("\n");
		mvOsOutput("%s %d:\n", name, unit);
		mvOsOutput("----\n");

		for (i = 0; i < 16; i++) {
			memset(&win, 0, sizeof(MV_UNIT_WIN_INFO));

			mvOsOutput("win%d - ", i);

			if (winGetFuncPtr(unit, i, &win) == MV_OK) {
				if (win.enable) {
					mvOsOutput("%s base %08x, ",
						   mvCtrlTargetNameGet(mvCtrlTargetByWinInfoGet(&win)),
						   win.addrWin.baseLow);
					mvOsOutput("....");
					if (win.addrWin.size == 0)
						mvOsOutput("size %3dGB ", 4);
					else
						mvSizePrint(win.addrWin.size);
					mvOsOutput("\n");
				} else
					mvOsOutput("disable\n");
			}
		}
	}
}

MV_VOID mvCtrlAddrDecShow(MV_VOID)
{
	mvCpuIfAddDecShow();
	mvAhbToMbusAddDecShow();
#if defined(MV_INCLUDE_PEX)
	mvCtrlPexAddrDecShow();
#endif

#if defined(MV_INCLUDE_USB)
	mvUnitAddrDecShow(mvCtrlUsbMaxGet(), USB_UNIT_ID, "USB", mvUsbWinRead);
#endif

#if defined(MV_INCLUDE_XOR)
	mvUnitAddrDecShow(mvCtrlXorMaxChanGet(), XOR_UNIT_ID, "XOR", mvXorTargetWinRead);
#endif

#if defined(MV_INCLUDE_SATA)
	mvUnitAddrDecShow(mvCtrlSataMaxPortGet(), SATA_UNIT_ID, "Sata", mvSataWinRead);
#endif
}

MV_U32 ctrlSizeToReg(MV_U32 size, MV_U32 alignment)
{
	MV_U32 retVal;

	if ((0 == size) || (MV_IS_NOT_ALIGN(size, alignment))) {
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Size is zero or not aligned.\n"));
		return -1;
	}

	alignment--;                     
	 
	while (alignment & 1) {          
		size = (size >> 1);      
		alignment = (alignment >> 1);
	}

	if (alignment) {
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Alignment parameter 0x%x invalid.\n", (MV_U32)alignment));
		return -1;
	}

	size--;                  
	retVal = size;

	while (size & 1)                 
		size = (size >> 1);      

	if (size) {                      
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Size parameter 0x%x invalid.\n", size));
		return -1;
	}
	return retVal;
}

MV_U32 ctrlRegToSize(MV_U32 regSize, MV_U32 alignment)
{
	MV_U32 temp;

	temp = regSize;                  

	while (temp & 1)                 
		temp = (temp >> 1);      

	if (temp) {                      
		DB(mvOsPrintf("%s: ERR: Size parameter 0x%x invalid.\n", __func__, regSize));
		return -1;
	}

	temp = alignment - 1;            

	while (temp & 1)                 
		temp = (temp >> 1);      

	if (temp) {
		DB(mvOsPrintf("%s: ERR: Alignment parameter 0x%x invalid.\n", __func__, alignment));
		return -1;
	}

	regSize++;               

	alignment--;                             

	while (alignment & 1) {                  
		regSize = (regSize << 1);        
		alignment = (alignment >> 1);
	}

	return regSize;
}

MV_U32 ctrlSizeRegRoundUp(MV_U32 size, MV_U32 alignment)
{
	MV_U32 msbBit = 0;
	MV_U32 retSize;

	if (!(-1 == ctrlSizeToReg(size, alignment)))
		return size;

	while (size) {
		size = (size >> 1);
		msbBit++;
	}

	retSize = (1 << msbBit);

	if (retSize < alignment)
		return alignment;
	else
		return retSize;
}

MV_BOOL mvCtrlIsBootFromNOR(MV_VOID)
{
	return MV_TRUE;
}

MV_BOOL mvCtrlIsBootFromSPI(MV_VOID)
{
	return MV_TRUE;  
}

MV_BOOL mvCtrlIsBootFromNAND(MV_VOID)
{
	return MV_FALSE;
}
 
MV_BOOL mvCtrlIsDLBEnabled(MV_VOID)
{
	MV_U32 reg;

	reg = MV_REG_READ(REG_STATIC_DRAM_DLB_CONTROL);

	return (reg & 0x1) ? MV_TRUE : MV_FALSE;
}

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
 
MV_VOID mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	 
	if (mvCtrlModelGet() == MV_FPGA_DEV_ID)
		return;

	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_PEX_STOP_CLK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_PEX_STOP_CLK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_INTEG_SATA)
	case SATA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SATA_STOP_CLK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SATA_STOP_CLK_MASK);

		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_USB_STOP_CLK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_USB_STOP_CLK_MASK);

		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SDIO_STOP_CLK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SDIO_STOP_CLK_MASK);

		break;
#endif
	case TDM_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_TDM_STOP_CLK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_TDM_STOP_CLK_MASK);
		break;
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_CESA_STOP_CLK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_CESA_STOP_CLK_MASK(index));
		break;
#endif
	default:
		break;
	}
}

MV_BOOL mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_BOOL state = MV_TRUE;

	if (mvCtrlModelGet() == MV_FPGA_DEV_ID)
		return MV_TRUE;

	MV_U32 reg = MV_REG_READ(POWER_MNG_CTRL_REG);
	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if ((reg & PMC_PEX_STOP_CLK_MASK(index)) == PMC_PEX_STOP_CLK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		if ((reg & PMC_SATA_STOP_CLK_MASK) == PMC_SATA_STOP_CLK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if ((reg & PMC_USB_STOP_CLK_MASK) == PMC_USB_STOP_CLK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if ((reg & PMC_SDIO_STOP_CLK_MASK) == PMC_SDIO_STOP_CLK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TDM)
	case TDM_UNIT_ID:
		if ((reg & PMC_TDM_STOP_CLK_MASK) == PMC_TDM_STOP_CLK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if ((reg & PMC_CESA_STOP_CLK_MASK(index)) == PMC_CESA_STOP_CLK_MASK(index))
			state = MV_TRUE;
		else
			state = MV_FALSE;
		break;
#endif
	default:
		state = MV_TRUE;
		break;
	}

	return state;
}

#else
MV_VOID mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	return;
}

MV_BOOL mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	return MV_TRUE;
}

#endif  

MV_U32 mvCtrlDDRBudWidth(MV_VOID)
{
	MV_U32 reg;

	reg = MV_REG_READ(REG_SDRAM_CONFIG_ADDR);

	return (reg & (0x1 << REG_SDRAM_CONFIG_DDR_BUS_OFFS)) ? 32 : 16;
}

MV_BOOL mvCtrlDDRThruXbar(MV_VOID)
{
	MV_U32 reg;

	reg = MV_REG_READ(0x20184);

	return (reg & 0x1) ? MV_FALSE : MV_TRUE;
}

MV_BOOL mvCtrlDDRECC(MV_VOID)
{
	MV_U32 reg;

	reg = MV_REG_READ(REG_SDRAM_CONFIG_ADDR);

	return (reg & (0x1 << REG_SDRAM_CONFIG_ECC_OFFS)) ? MV_TRUE : MV_FALSE;
}


#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
MV_32 mvCtrlGetJuncTemp(MV_VOID)
#else
MV_U32 mvCtrlGetJuncTemp(MV_VOID)
#endif
{
	MV_32 reg = 0;
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
	 
	static MV_32 reg_last = 249;
#endif

	reg = MV_REG_READ(TSEN_CTRL_MSB_REG);
	 
	reg &= ~TSEN_CTRL_UNIT_CTRL_MASK;
	reg |= (0x0 << TSEN_CTRL_UNIT_CTRL_OFFSET);
	 
	reg &= ~TSEN_CTRL_READOUT_INVERT_MASK;
	reg |= (0x0 << TSEN_CTRL_READOUT_INVERT_OFFSET);
	 
	reg &= ~TSEN_CTRL_SOFT_RST_MASK;
	reg |= (0x0 << TSEN_CTRL_SOFT_RST_OFFSET);
	MV_REG_WRITE(TSEN_CTRL_MSB_REG, reg);
	mvOsDelay(20);
	 
	reg &= ~TSEN_CTRL_SOFT_RST_MASK;
	reg |= (0x1 << TSEN_CTRL_SOFT_RST_OFFSET);
	MV_REG_WRITE(TSEN_CTRL_MSB_REG, reg);

	mvOsDelay(50);
	 
	reg = MV_REG_READ(TSEN_STATUS_REG);
	reg = (reg & TSEN_STATUS_TEMP_OUT_MASK) >> TSEN_STATUS_TEMP_OUT_OFFSET;

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
	if (0 == reg)
		reg = reg_last;
	else
		reg_last = reg;
#endif

	return (3239600 - (10000 * reg)) / 13616;
}
 
void mvCtrlNandClkSet(int nClock)
{
	 
	MV_U32 nVal = MV_REG_READ(CORE_DIV_CLK_CTRL(1));
	nVal &= ~(NAND_ECC_DIVCKL_RATIO_MASK);
	nVal |= (nClock << NAND_ECC_DIVCKL_RATIO_OFFS);
	MV_REG_WRITE(CORE_DIV_CLK_CTRL(1), nVal);

	nVal = MV_REG_READ(CORE_DIV_CLK_CTRL(0));
	nVal &= ~(CORE_DIVCLK_RELOAD_FORCE_MASK);
	nVal |= CORE_DIVCLK_RELOAD_FORCE_VAL;
	MV_REG_WRITE(CORE_DIV_CLK_CTRL(0), nVal);

	MV_REG_BIT_SET(CORE_DIV_CLK_CTRL(0), CORE_DIVCLK_RELOAD_RATIO_MASK);
	mvOsDelay(1);  
	 
	MV_REG_BIT_RESET(CORE_DIV_CLK_CTRL(0), CORE_DIVCLK_RELOAD_RATIO_MASK);
}
